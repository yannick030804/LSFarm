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

#define MSG_INIT_OK        "INIT OK\r\n"
#define MSG_INIT_ERROR     "INIT ERROR\r\n"
#define MSG_SLEEP_OK       "SLEEP OK\r\n"
#define MSG_SLEEP_ERROR    "SLEEP ERROR\r\n"
#define MSG_RESET_OK       "RESET OK\r\n"
#define MSG_REBELLION_ON   "REBELLION ON\r\n"
#define MSG_REBELLION_OFF  "REBELLION OFF\r\n"
#define MSG_CONSUME_OK     "CONSUME OK\r\n"
#define MSG_CONSUME_ERROR  "CONSUME ERROR\r\n"
#define MSG_BUTTON         "S\r\n"
#define MSG_FINISH_ANIMALS "F\r\n"
#define MSG_UP             "U\r\n"
#define MSG_DOWN           "D\r\n"
#define MSG_LEFT           "L\r\n"
#define MSG_RIGHT          "R\r\n"

static const char *txLine;
static char farmName[17];
static char txBuffer[21];
static unsigned char controllerFlags;
static unsigned char persistenceIndex;

static unsigned char isDigit (char c) {
    if (c >= '0' && c <= '9') {
        return 1;
    }
    return 0;
}

static unsigned char isValidGenerationTime (unsigned char value) {
    if (value >= 1 && value <= 60) {
        return 1;
    }
    return 0;
}

static unsigned char appendNum (unsigned char index, unsigned char value) {
    unsigned char d;
    unsigned char started = 0;

    d = (unsigned char)(value / 100);
    if (d != 0) {
        txBuffer[index++] = (char)('0' + d);
        started = 1;
    }

    d = (unsigned char)((value / 10) % 10);
    if (d != 0 || started == 1) {
        txBuffer[index++] = (char)('0' + d);
    }

    txBuffer[index++] = (char)('0' + (value % 10));
    return index;
}

static unsigned char parseNumber (const char *text, unsigned char *index, unsigned char *value, char endChar) {
    unsigned char number = 0;
    unsigned char hasDigits = 0;

    while (isDigit(text[*index])) {
        number = (unsigned char)(number * 10 + (text[*index] - '0'));
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
        if (nameIndex < sizeof(farmName) - 1) {
            farmName[nameIndex++] = text[i];
        }
        i++;
    }

    if (nameIndex == 0 || text[i] != '$') {
        return 0;
    }

    farmName[nameIndex] = '\0';
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

    if (isValidGenerationTime(cow) == 0 || isValidGenerationTime(horse) == 0 ||
        isValidGenerationTime(pig) == 0 || isValidGenerationTime(chicken) == 0) {
        return 0;
    }

    Farm_RequestConfigure(farmName, cow, horse, pig, chicken);
    return 1;
}

static unsigned char parseSleepCommand (const char *text) {
    unsigned char i;
    unsigned char species;
    unsigned char number;

    if (text[2] == 'V' && text[3] == 'A' && text[4] == 'C' && text[5] == 'A' && text[6] == '$') {
        species = 0;
        i = 7;
    } else if (text[2] == 'P' && text[3] == 'O' && text[4] == 'R' && text[5] == 'C' && text[6] == '$') {
        species = 1;
        i = 7;
    } else if (text[2] == 'C' && text[3] == 'A' && text[4] == 'V' && text[5] == 'A' &&
               text[6] == 'L' && text[7] == 'L' && text[8] == '$') {
        species = 2;
        i = 9;
    } else if (text[2] == 'G' && text[3] == 'A' && text[4] == 'L' && text[5] == 'L' &&
               text[6] == 'I' && text[7] == 'N' && text[8] == 'A' && text[9] == '$') {
        species = 3;
        i = 10;
    } else {
        return 0;
    }

    if (parseNumber(text, &i, &number, '\0') == 0 || number == 0) {
        return 0;
    }

    Farm_RequestSelectAnimal(species, number);
    return 1;
}

static void Controller_ServiceFarm (void) {
    unsigned char timeConfigured = SerialTime_IsConfigured();

    if (timeConfigured == 1) {
        Farm_SetCurrentDate(1, SerialTime_GetDay(), SerialTime_GetMonth(), SerialTime_GetHour(), SerialTime_GetMinute(), SerialTime_GetSecond());
    } else {
        Farm_SetCurrentDate(0, 0, 0, 0, 0, 0);
    }

    if ((controllerFlags & CTRL_FLAG_PERSISTENCE_LOADED) == 0 && EEPROM_IsBusy() == 0) {
        if (EEPROM_ReadByte(0) == FARM_STATE_MAGIC) {
            Farm_BeginImportState();
            for (persistenceIndex = 0; persistenceIndex < FARM_STATE_SIZE; persistenceIndex++) {
                Farm_ImportStateByte(persistenceIndex, EEPROM_ReadByte((unsigned char)(persistenceIndex + 1)));
            }
            Farm_EndImportState();
        }
        controllerFlags |= CTRL_FLAG_PERSISTENCE_LOADED;
    }

    if ((controllerFlags & CTRL_FLAG_RESET_PENDING) != 0 && EEPROM_IsBusy() == 0) {
        EEPROM_RequestClear();
        controllerFlags &= (unsigned char)(~CTRL_FLAG_RESET_PENDING);
        return;
    }

    if ((controllerFlags & CTRL_FLAG_SAVE_ACTIVE) != 0 && EEPROM_IsBusy() == 0) {
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

    if (Farm_IsDirty() == 1 && EEPROM_IsBusy() == 0 && Farm_IsConfigured() == 1) {
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
    const char *name;

    Farm_GetAnimal(indexAnimal, &species, &number, &critical);

    txBuffer[0] = 'A';
    txBuffer[1] = ':';

    if (species == 0) {
        name = "VACA";
    } else if (species == 1) {
        name = "PORC";
    } else if (species == 2) {
        name = "CAVALL";
    } else {
        name = "GALLINA";
    }

    while (*name != '\0') {
        txBuffer[index++] = *name++;
    }

    txBuffer[index++] = '$';
    index = appendNum(index, number);
    txBuffer[index++] = '$';

    if (critical == 1) {
        name = "SLEEP";
    } else {
        name = "AWAKE";
    }

    while (*name != '\0') {
        txBuffer[index++] = *name++;
    }

    txBuffer[index++] = '\r';
    txBuffer[index++] = '\n';
    txBuffer[index] = '\0';
}

void Controller_Init (void) {
    farmName[0] = '\0';
    controllerFlags = 0;
    persistenceIndex = 0;
}

void motorController (void) {
    static unsigned char state = 0;
    static unsigned char animalIndex = 0;
    const char *line;
    unsigned char recipeId;

    switch (state) {
        case 0:
            Controller_ServiceFarm();
            line = SJ_GetLine();
            if (line != 0) {
                if (line[0] == 'I' && line[1] == ':') {
                    if (parseInitCommand(line) == 1) {
                        state = 1;
                    } else {
                        txLine = MSG_INIT_ERROR;
                        state = 5;
                    }
                } else if (line[0] == 'S' && line[1] == ':') {
                    if (parseSleepCommand(line) == 1) {
                        state = 2;
                    } else {
                        txLine = MSG_SLEEP_ERROR;
                        state = 5;
                    }
                } else if (line[0] == 'G' && line[1] == '\0') {
                    buildProductsLine();
                    txLine = txBuffer;
                    state = 5;
                } else if (line[0] == 'A' && line[1] == '\0') {
                    animalIndex = 0;
                    state = 4;
                } else if (line[0] == 'R' && line[1] == '\0') {
                    Farm_Reset();
                    Heartbeat_SetRebellion(0);
                    controllerFlags |= CTRL_FLAG_RESET_PENDING;
                    txLine = MSG_RESET_OK;
                    state = 5;
                } else if (line[0] == 'B' && line[1] == '\0') {
                    Farm_SetRebellion(1);
                    Heartbeat_SetRebellion(1);
                    txLine = MSG_REBELLION_ON;
                    state = 5;
                } else if (line[0] == 'P' && line[1] == '\0') {
                    Farm_SetRebellion(0);
                    Heartbeat_SetRebellion(0);
                    txLine = MSG_REBELLION_OFF;
                    state = 5;
                } else if (line[0] == 'C' && line[1] == ':' && isDigit(line[2]) && line[3] == '\0') {
                    recipeId = (unsigned char)(line[2] - '0');
                    if (recipeId <= 3) {
                        Farm_Consume(recipeId);
                        txLine = MSG_CONSUME_OK;
                    } else {
                        txLine = MSG_CONSUME_ERROR;
                    }
                    state = 5;
                } else {
                    txLine = MSG_INIT_ERROR;
                    state = 5;
                }
            } else if (getButton() == 1) {
                txLine = MSG_BUTTON;
                state = 5;
            } else {
                recipeId = Joystick_GetEvent();
                if (recipeId == JOY_EVT_UP) {
                    txLine = MSG_UP;
                    state = 5;
                } else if (recipeId == JOY_EVT_DOWN) {
                    txLine = MSG_DOWN;
                    state = 5;
                } else if (recipeId == JOY_EVT_LEFT) {
                    txLine = MSG_LEFT;
                    state = 5;
                } else if (recipeId == JOY_EVT_RIGHT) {
                    txLine = MSG_RIGHT;
                    state = 5;
                }
            }
            break;

        case 1:
            if (Farm_IsConfigured() == 1) {
                txLine = MSG_INIT_OK;
                state = 5;
            }
            break;

        case 2:
            if (Farm_IsSearchFinished() == 1) {
                if (Farm_IsAnimalFound() == 1) {
                    state = 3;
                } else {
                    txLine = MSG_SLEEP_ERROR;
                    state = 5;
                }
            }
            break;

        case 3:
            if (Farm_IsRestFinished() == 1) {
                if (Farm_IsRestSuccess() == 1) {
                    txLine = MSG_SLEEP_OK;
                } else {
                    txLine = MSG_SLEEP_ERROR;
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
                txLine = MSG_FINISH_ANIMALS;
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
