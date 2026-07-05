#include <xc.h>
#include "TAD_BUTTON.h"
#include "TAD_CONTROLLER.h"
#include "TAD_EEPROM.h"
#include "TAD_FARM.h"
#include "TAD_HEARTBEAT.h"
#include "TAD_JOYSTICK.h"
#include "TAD_SERIAL_JAVA.h"
#include "TAD_SERIAL_TIME.h"

#define FARM_STATE_MAGIC 0xA5
#define CTRL_FLAG_PERSISTENCE_LOADED 0x01
#define CTRL_FLAG_RESET_PENDING      0x02
#define CTRL_FLAG_SAVE_ACTIVE        0x04

#define REPLY_INIT_OK       0
#define REPLY_INIT_ERROR    1
#define REPLY_SLEEP_OK      2
#define REPLY_SLEEP_ERROR   3
#define REPLY_RESET_OK      4
#define REPLY_REBELLION_ON  5
#define REPLY_REBELLION_OFF 6
#define REPLY_CONSUME_OK    7
#define REPLY_CONSUME_ERROR 8
#define REPLY_BUTTON        9
#define REPLY_FINISH_ANIMALS 10
#define REPLY_UP            11
#define REPLY_DOWN          12
#define REPLY_LEFT          13
#define REPLY_RIGHT         14

static const char *txLine;
static char controllerFarmName[17];
static char txBuffer[21];
static unsigned char controllerFlags;
static unsigned char persistenceIndex;

static unsigned char isDigit (char c) {
    if (c >= '0' && c <= '9') {
        return 1;
    }
    return 0;
}

static unsigned char appendNum (unsigned char index, unsigned char value) {
    unsigned char digit = 0;

    if (value >= 100) {
        while (value >= 100) {
            value = (unsigned char)(value - 100);
            digit++;
        }
        txBuffer[index++] = (char)('0' + digit);
        digit = 0;
        while (value >= 10) {
            value = (unsigned char)(value - 10);
            digit++;
        }
        txBuffer[index++] = (char)('0' + digit);
    } else if (value >= 10) {
        while (value >= 10) {
            value = (unsigned char)(value - 10);
            digit++;
        }
        txBuffer[index++] = (char)('0' + digit);
    }

    txBuffer[index++] = (char)('0' + value);
    return index;
}

static void Controller_EndReply (unsigned char index) {
    txBuffer[index++] = '\r';
    txBuffer[index++] = '\n';
    txBuffer[index] = '\0';
    txLine = txBuffer;
}

static void Controller_SetReply (unsigned char reply) {
    unsigned char index = 0;

    switch (reply) {
        case REPLY_INIT_OK:
        case REPLY_INIT_ERROR:
            txBuffer[index++] = 'I';
            txBuffer[index++] = 'N';
            txBuffer[index++] = 'I';
            txBuffer[index++] = 'T';
            if (reply == REPLY_INIT_OK) {
                goto appendOk;
            }
            goto appendError;

        case REPLY_SLEEP_OK:
        case REPLY_SLEEP_ERROR:
            txBuffer[index++] = 'S';
            txBuffer[index++] = 'L';
            txBuffer[index++] = 'E';
            txBuffer[index++] = 'E';
            txBuffer[index++] = 'P';
            if (reply == REPLY_SLEEP_OK) {
                goto appendOk;
            }
            goto appendError;

        case REPLY_RESET_OK:
            txBuffer[index++] = 'R';
            txBuffer[index++] = 'E';
            txBuffer[index++] = 'S';
            txBuffer[index++] = 'E';
            txBuffer[index++] = 'T';
            goto appendOk;

        case REPLY_REBELLION_ON:
        case REPLY_REBELLION_OFF:
            txBuffer[index++] = 'R';
            txBuffer[index++] = 'E';
            txBuffer[index++] = 'B';
            txBuffer[index++] = 'E';
            txBuffer[index++] = 'L';
            txBuffer[index++] = 'L';
            txBuffer[index++] = 'I';
            txBuffer[index++] = 'O';
            txBuffer[index++] = 'N';
            if (reply == REPLY_REBELLION_ON) {
                goto appendOn;
            }
            goto appendOff;

        case REPLY_CONSUME_OK:
        case REPLY_CONSUME_ERROR:
            txBuffer[index++] = 'C';
            txBuffer[index++] = 'O';
            txBuffer[index++] = 'N';
            txBuffer[index++] = 'S';
            txBuffer[index++] = 'U';
            txBuffer[index++] = 'M';
            txBuffer[index++] = 'E';
            if (reply == REPLY_CONSUME_OK) {
                goto appendOk;
            }
            goto appendError;

        case REPLY_BUTTON:
            txBuffer[index++] = 'S';
            break;
        case REPLY_FINISH_ANIMALS:
            txBuffer[index++] = 'F';
            break;
        case REPLY_UP:
            txBuffer[index++] = 'U';
            break;
        case REPLY_DOWN:
            txBuffer[index++] = 'D';
            break;
        case REPLY_LEFT:
            txBuffer[index++] = 'L';
            break;
        case REPLY_RIGHT:
            txBuffer[index++] = 'R';
            break;
        default:
            txBuffer[index++] = 'R';
            break;
    }

    goto endReply;

appendOk:
    txBuffer[index++] = ' ';
    txBuffer[index++] = 'O';
    txBuffer[index++] = 'K';
    goto endReply;

appendError:
    txBuffer[index++] = ' ';
    txBuffer[index++] = 'E';
    txBuffer[index++] = 'R';
    txBuffer[index++] = 'R';
    txBuffer[index++] = 'O';
    txBuffer[index++] = 'R';
    goto endReply;

appendOn:
    txBuffer[index++] = ' ';
    txBuffer[index++] = 'O';
    txBuffer[index++] = 'N';
    goto endReply;

appendOff:
    txBuffer[index++] = ' ';
    txBuffer[index++] = 'O';
    txBuffer[index++] = 'F';
    txBuffer[index++] = 'F';

endReply:
    Controller_EndReply(index);
}

static unsigned char parseNumber (const char *text, unsigned char *index, unsigned char *value, char endChar) {
    unsigned char number = 0;
    unsigned char hasDigits = 0;

    while (isDigit(text[*index])) {
        number = (unsigned char)((number << 3) + (number << 1) + (unsigned char)(text[*index] - '0'));
        (*index)++;
        hasDigits = 1;
    }

    if (hasDigits == 0 || text[*index] != endChar) {
        return 0;
    }

    *value = number;
    return 1;
}

static unsigned char parseInitCommand (const char *text) {
    unsigned char i = 2;
    unsigned char nameIndex = 0;
    unsigned char cow;
    unsigned char horse;
    unsigned char pig;
    unsigned char chicken;

    while (text[i] != '$' && text[i] != '\0') {
        if (nameIndex < sizeof(controllerFarmName) - 1) {
            controllerFarmName[nameIndex++] = text[i];
        }
        i++;
    }

    if (nameIndex == 0 || text[i] != '$') {
        return 0;
    }

    controllerFarmName[nameIndex] = '\0';
    i++;

    if (parseNumber(text, &i, &cow, '$') == 0) {
        return 0;
    }
    i++;
    if (parseNumber(text, &i, &horse, '$') == 0) {
        return 0;
    }
    i++;
    if (parseNumber(text, &i, &pig, '$') == 0) {
        return 0;
    }
    i++;
    if (parseNumber(text, &i, &chicken, '\0') == 0) {
        return 0;
    }

    if (cow < 1 || cow > 60 || horse < 1 || horse > 60 ||
        pig < 1 || pig > 60 || chicken < 1 || chicken > 60) {
        return 0;
    }

    Farm_RequestConfigure(controllerFarmName, cow, horse, pig, chicken);
    return 1;
}

static unsigned char parseSleepCommand (const char *text) {
    unsigned char i;
    unsigned char species;
    unsigned char number;

    switch (text[2]) {
        case 'V':
            if (text[3] == 'A' && text[4] == 'C' && text[5] == 'A' && text[6] == '$') {
                species = 0;
                i = 7;
                break;
            }
            return 0;
        case 'P':
            if (text[3] == 'O' && text[4] == 'R' && text[5] == 'C' && text[6] == '$') {
                species = 1;
                i = 7;
                break;
            }
            return 0;
        case 'C':
            if (text[3] == 'A' && text[4] == 'V' && text[5] == 'A' &&
                text[6] == 'L' && text[7] == 'L' && text[8] == '$') {
                species = 2;
                i = 9;
                break;
            }
            return 0;
        case 'G':
            if (text[3] == 'A' && text[4] == 'L' && text[5] == 'L' &&
                text[6] == 'I' && text[7] == 'N' && text[8] == 'A' && text[9] == '$') {
                species = 3;
                i = 10;
                break;
            }
            return 0;
        default:
            return 0;
    }

    if (parseNumber(text, &i, &number, '\0') == 0 || number == 0) {
        return 0;
    }

    Farm_RequestSelectAnimal(species, number);
    return 1;
}

static void Controller_ServiceFarm (void) {
    if (SerialTime_IsConfigured() == 1) {
        Farm_SetCurrentDate(1, SerialTime_GetDay(), SerialTime_GetMonth(), SerialTime_GetHour(), SerialTime_GetMinute(), SerialTime_GetSecond());
    } else {
        Farm_SetCurrentDate(0, 0, 0, 0, 0, 0);
    }

    if (EEPROM_IsBusy() != 0) {
        return;
    }

    if ((controllerFlags & CTRL_FLAG_PERSISTENCE_LOADED) == 0) {
        if (EEPROM_ReadByte(0) == FARM_STATE_MAGIC) {
            Farm_BeginImportState();
            for (persistenceIndex = 0; persistenceIndex < FARM_STATE_SIZE; persistenceIndex++) {
                Farm_ImportStateByte(persistenceIndex, EEPROM_ReadByte((unsigned char)(persistenceIndex + 1)));
            }
            Farm_EndImportState();
        }
        controllerFlags |= CTRL_FLAG_PERSISTENCE_LOADED;
    }

    if ((controllerFlags & CTRL_FLAG_RESET_PENDING) != 0) {
        EEPROM_RequestClear();
        controllerFlags &= (unsigned char)(~CTRL_FLAG_RESET_PENDING);
        return;
    }

    if ((controllerFlags & CTRL_FLAG_SAVE_ACTIVE) != 0) {
        if (persistenceIndex == 0) {
            if (EEPROM_StartByteWrite(0, FARM_STATE_MAGIC) == 1) {
                persistenceIndex = 1;
            }
        } else if (persistenceIndex <= FARM_STATE_SIZE) {
            if (EEPROM_StartByteWrite(persistenceIndex, Farm_ExportByte((unsigned char)(persistenceIndex - 1))) == 1) {
                persistenceIndex++;
            }
        } else {
            controllerFlags &= (unsigned char)(~CTRL_FLAG_SAVE_ACTIVE);
            Farm_ClearDirty();
        }
        return;
    }

    if (Farm_IsDirty() == 1 && Farm_IsConfigured() == 1) {
        controllerFlags |= CTRL_FLAG_SAVE_ACTIVE;
        persistenceIndex = 0;
    }
}

static void buildProductsLine (void) {
    unsigned char index = 2;

    txBuffer[0] = 'P';
    txBuffer[1] = ':';
    index = appendNum(index, Farm_GetProductTotal(0));
    txBuffer[index++] = '$';
    index = appendNum(index, Farm_GetProductTotal(1));
    txBuffer[index++] = '$';
    index = appendNum(index, Farm_GetProductTotal(3));
    txBuffer[index++] = '$';
    index = appendNum(index, Farm_GetProductTotal(2));
    txBuffer[index++] = '\r';
    txBuffer[index++] = '\n';
    txBuffer[index] = '\0';
}

static void buildAnimalLine (unsigned char indexAnimal) {
    unsigned char species;
    unsigned char number;
    unsigned char critical;
    unsigned char index = 2;

    Farm_GetAnimal(indexAnimal, &species, &number, &critical);

    txBuffer[0] = 'A';
    txBuffer[1] = ':';

    if (species == 0) {
        txBuffer[index++] = 'V';
        txBuffer[index++] = 'A';
        txBuffer[index++] = 'C';
        txBuffer[index++] = 'A';
    } else if (species == 1) {
        txBuffer[index++] = 'P';
        txBuffer[index++] = 'O';
        txBuffer[index++] = 'R';
        txBuffer[index++] = 'C';
    } else if (species == 2) {
        txBuffer[index++] = 'C';
        txBuffer[index++] = 'A';
        txBuffer[index++] = 'V';
        txBuffer[index++] = 'A';
        txBuffer[index++] = 'L';
        txBuffer[index++] = 'L';
    } else {
        txBuffer[index++] = 'G';
        txBuffer[index++] = 'A';
        txBuffer[index++] = 'L';
        txBuffer[index++] = 'L';
        txBuffer[index++] = 'I';
        txBuffer[index++] = 'N';
        txBuffer[index++] = 'A';
    }

    txBuffer[index++] = '$';
    index = appendNum(index, number);
    txBuffer[index++] = '$';

    if (critical == 1) {
        txBuffer[index++] = 'S';
        txBuffer[index++] = 'L';
        txBuffer[index++] = 'E';
        txBuffer[index++] = 'E';
        txBuffer[index++] = 'P';
    } else {
        txBuffer[index++] = 'A';
        txBuffer[index++] = 'W';
        txBuffer[index++] = 'A';
        txBuffer[index++] = 'K';
        txBuffer[index++] = 'E';
    }

    txBuffer[index++] = '\r';
    txBuffer[index++] = '\n';
    txBuffer[index] = '\0';
}

static unsigned char Controller_ProcessLine (const char *line, unsigned char *animalIndex) {
    unsigned char recipeId;

    Controller_SetReply(REPLY_INIT_ERROR);

    switch (line[0]) {
        case 'I':
            if (line[1] == ':' && parseInitCommand(line) == 1) {
                return 1;
            }
            break;
        case 'S':
            if (line[1] == ':' && parseSleepCommand(line) == 1) {
                return 2;
            }
            Controller_SetReply(REPLY_SLEEP_ERROR);
            break;
        case 'G':
            if (line[1] == '\0') {
                buildProductsLine();
                txLine = txBuffer;
            }
            break;
        case 'A':
            if (line[1] == '\0') {
                *animalIndex = 0;
                return 4;
            }
            break;
        case 'R':
            if (line[1] == '\0') {
                Farm_Reset();
                Heartbeat_SetRebellion(0);
                controllerFlags |= CTRL_FLAG_RESET_PENDING;
                Controller_SetReply(REPLY_RESET_OK);
            }
            break;
        case 'B':
            if (line[1] == '\0') {
                Farm_SetRebellion(1);
                Heartbeat_SetRebellion(1);
                Controller_SetReply(REPLY_REBELLION_ON);
            }
            break;
        case 'P':
            if (line[1] == '\0') {
                Farm_SetRebellion(0);
                Heartbeat_SetRebellion(0);
                Controller_SetReply(REPLY_REBELLION_OFF);
            }
            break;
        case 'C':
            if (line[1] == ':' && line[3] == '\0') {
                recipeId = (unsigned char)(line[2] - '0');
                if (recipeId <= 3) {
                    Farm_Consume(recipeId);
                    Controller_SetReply(REPLY_CONSUME_OK);
                } else {
                    Controller_SetReply(REPLY_CONSUME_ERROR);
                }
            }
            break;
    }

    return 5;
}

static unsigned char Controller_ProcessInputs (void) {
    unsigned char recipeId;

    if (getButton() == 1) {
        Controller_SetReply(REPLY_BUTTON);
        return 5;
    }

    recipeId = Joystick_GetEvent();
    if (recipeId != JOY_EVT_NONE) {
        Controller_SetReply((unsigned char)(REPLY_FINISH_ANIMALS + recipeId));
        return 5;
    }

    return 0;
}

void motorController (void) {
    static unsigned char state = 0;
    static unsigned char animalIndex = 0;
    const char *line;

    switch (state) {
        case 0:
            Controller_ServiceFarm();
            line = SJ_GetLine();
            if (line != 0) {
                state = Controller_ProcessLine(line, &animalIndex);
            } else {
                state = Controller_ProcessInputs();
            }
            break;

        case 1:
            if (Farm_IsConfigured() == 1) {
                Controller_SetReply(REPLY_INIT_OK);
                state = 5;
            }
            break;

        case 2:
            if (Farm_IsSearchFinished() == 1) {
                if (Farm_IsAnimalFound() == 1) {
                    state = 3;
                } else {
                    Controller_SetReply(REPLY_SLEEP_ERROR);
                    state = 5;
                }
            }
            break;

        case 3:
            if (Farm_IsRestFinished() == 1) {
                if (Farm_IsRestSuccess() == 1) {
                    Controller_SetReply(REPLY_SLEEP_OK);
                } else {
                    Controller_SetReply(REPLY_SLEEP_ERROR);
                }
                state = 5;
            }
            break;

        case 4:
            if (animalIndex < Farm_GetAnimalCount()) {
                buildAnimalLine(animalIndex);
                txLine = txBuffer;
                state = 6;
            } else {
                Controller_SetReply(REPLY_FINISH_ANIMALS);
                state = 5;
            }
            break;

        case 5:
            if (SJ_PutString(txLine) == 1) {
                state = 0;
            }
            break;

        case 6:
            if (SJ_PutString(txLine) == 1) {
                animalIndex++;
                state = 4;
            }
            break;
    }
}
