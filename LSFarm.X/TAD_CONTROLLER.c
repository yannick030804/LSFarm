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
#define CTRL_FLAG_INIT_SENT          0x01
#define CTRL_FLAG_PERSISTENCE_LOADED 0x02
#define CTRL_FLAG_RESET_PENDING      0x04
#define CTRL_FLAG_SAVE_ACTIVE        0x08

#define SPECIES_INVALID 255

static const char *line;
static const char *txLine;
static char farmName[17];
static unsigned char params[4];
static unsigned char farmNameIndex;
static unsigned char selectedNumber;
static unsigned char selectedSpecies;
static unsigned char sleepTypeStart;
static unsigned char joystickEvent;
static unsigned char animalIndex;
static unsigned char recipeId;
static char txBuffer[21];
static unsigned char controllerFlags;
static unsigned char persistenceIndex;

static void resetInitData (void) {
    farmNameIndex = 0;
    farmName[0] = '\0';
    params[0] = 0;
    params[1] = 0;
    params[2] = 0;
    params[3] = 0;
    selectedNumber = 0;
    selectedSpecies = SPECIES_INVALID;
    sleepTypeStart = 0;
    controllerFlags &= (unsigned char)(~CTRL_FLAG_INIT_SENT);
}

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

static unsigned char getSpeciesFromRange (const char *text, unsigned char start, unsigned char end) {
    unsigned char len = (unsigned char)(end - start);

    if (len == 4 &&
        text[start] == 'V' &&
        text[start + 1] == 'A' &&
        text[start + 2] == 'C' &&
        text[start + 3] == 'A') {
        return 0;
    }

    if (len == 4 &&
        text[start] == 'P' &&
        text[start + 1] == 'O' &&
        text[start + 2] == 'R' &&
        text[start + 3] == 'C') {
        return 1;
    }

    if (len == 6 &&
        text[start] == 'C' &&
        text[start + 1] == 'A' &&
        text[start + 2] == 'V' &&
        text[start + 3] == 'A' &&
        text[start + 4] == 'L' &&
        text[start + 5] == 'L') {
        return 2;
    }

    if (len == 7 &&
        text[start] == 'G' &&
        text[start + 1] == 'A' &&
        text[start + 2] == 'L' &&
        text[start + 3] == 'L' &&
        text[start + 4] == 'I' &&
        text[start + 5] == 'N' &&
        text[start + 6] == 'A') {
        return 3;
    }

    return SPECIES_INVALID;
}

static unsigned char appendNum (unsigned char index, unsigned char value) {
    unsigned char digit;
    unsigned char started = 0;

    digit = (unsigned char)(value / 100);
    if (digit != 0) {
        txBuffer[index] = (char)('0' + digit);
        index++;
        started = 1;
    }

    digit = (unsigned char)((value / 10) % 10);
    if (digit != 0 || started == 1) {
        txBuffer[index] = (char)('0' + digit);
        index++;
    }

    txBuffer[index] = (char)('0' + (value % 10));
    index++;
    return index;
}

static void Controller_ServiceFarm (void) {
    Farm_SetTimeReady(SerialTime_IsConfigured());
    if (SerialTime_IsConfigured() == 1) {
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
    const char *text;

    Farm_GetAnimal(indexAnimal, &species, &number, &critical);

    txBuffer[0] = 'A';
    txBuffer[1] = ':';

    if (species == 0) {
        text = "VACA";
    } else if (species == 1) {
        text = "PORC";
    } else if (species == 2) {
        text = "CAVALL";
    } else {
        text = "GALLINA";
    }

    while (*text != '\0') {
        txBuffer[index++] = *text++;
    }

    txBuffer[index++] = '$';
    index = appendNum(index, number);
    txBuffer[index++] = '$';

    if (critical == 1) {
        text = "SLEEP";
    } else {
        text = "AWAKE";
    }

    while (*text != '\0') {
        txBuffer[index++] = *text++;
    }

    txBuffer[index++] = '\r';
    txBuffer[index++] = '\n';
    txBuffer[index] = '\0';
}

void Controller_Init (void) {
    resetInitData();
    controllerFlags = 0;
    persistenceIndex = 0;
}

void motorController (void) {
    static unsigned char state = 0;
    static unsigned char i = 0;

    switch (state) {
        case 0:
            Controller_ServiceFarm();
            line = SJ_GetLine();
            if (line != 0) {
                resetInitData();
                i = 0;
                state = 1;
            } else if (getButton() == 1) {
                txLine = "S\r\n";
                state = 15;
            } else {
                joystickEvent = Joystick_GetEvent();
                if (joystickEvent != JOY_EVT_NONE) {
                    state = 16;
                }
            }
            break;

        case 1:
            switch (line[0]) {
                case 'I':
                    if (line[1] == ':') {
                        i = 2;
                        state = 2;
                    } else {
                        state = 14;
                    }
                    break;

                case 'G':
                    if (line[1] == '\0') {
                        state = 11;
                    } else {
                        state = 14;
                    }
                    break;

                case 'A':
                    if (line[1] == '\0') {
                        animalIndex = 0;
                        state = 12;
                    } else {
                        state = 14;
                    }
                    break;

                case 'R':
                    if (line[1] == '\0') {
                        Farm_Reset();
                        Farm_SetRebellion(0);
                        Heartbeat_SetRebellion(0);
                        controllerFlags |= CTRL_FLAG_RESET_PENDING;
                        txLine = "RESET OK\r\n";
                        state = 15;
                    } else {
                        state = 14;
                    }
                    break;

                case 'B':
                    if (line[1] == '\0') {
                        Farm_SetRebellion(1);
                        Heartbeat_SetRebellion(1);
                        txLine = "REBELLION ON\r\n";
                        state = 15;
                    } else {
                        state = 14;
                    }
                    break;

                case 'P':
                    if (line[1] == '\0') {
                        Farm_SetRebellion(0);
                        Heartbeat_SetRebellion(0);
                        txLine = "REBELLION OFF\r\n";
                        state = 15;
                    } else {
                        state = 14;
                    }
                    break;

                case 'C':
                    if (line[1] == ':' && isDigit(line[2]) && line[3] == '\0') {
                        recipeId = (unsigned char)(line[2] - '0');
                        state = 13;
                    } else {
                        state = 14;
                    }
                    break;

                case 'S':
                    if (line[1] == ':') {
                        selectedNumber = 0;
                        selectedSpecies = SPECIES_INVALID;
                        sleepTypeStart = 2;
                        i = 2;
                        state = 5;
                    } else {
                        state = 14;
                    }
                    break;

                default:
                    state = 14;
                    break;
            }
            break;

        case 2:
            if (line[i] == '\0') {
                state = 14;
            } else if (line[i] == '$') {
                farmName[farmNameIndex] = '\0';
                i++;
                recipeId = 0;
                state = 3;
            } else {
                if (farmNameIndex < sizeof(farmName) - 1) {
                    farmName[farmNameIndex++] = line[i];
                }
                i++;
            }
            break;

        case 3:
            if (isDigit(line[i])) {
                params[recipeId] = (unsigned char)(params[recipeId] * 10 + (unsigned char)(line[i] - '0'));
                i++;
            } else if (line[i] == '$' && recipeId < 3) {
                recipeId++;
                i++;
            } else if (line[i] == '\0' && recipeId == 3) {
                state = 4;
            } else {
                state = 14;
            }
            break;

        case 4:
            if (farmName[0] == '\0') {
                state = 14;
                break;
            }

            for (recipeId = 0; recipeId < 4; recipeId++) {
                if (isValidGenerationTime(params[recipeId]) == 0) {
                    state = 14;
                    break;
                }
            }

            if (state == 14) {
                break;
            }

            if ((controllerFlags & CTRL_FLAG_INIT_SENT) == 0) {
                Farm_RequestConfigure(farmName, params[0], params[1], params[2], params[3]);
                controllerFlags |= CTRL_FLAG_INIT_SENT;
            }

            if (Farm_IsConfigured() == 1) {
                txLine = "INIT OK\r\n";
                state = 15;
            }
            break;

        case 5:
            if (line[i] == '\0') {
                txLine = "SLEEP ERROR\r\n";
                state = 15;
            } else if (line[i] == '$') {
                selectedSpecies = getSpeciesFromRange(line, sleepTypeStart, i);
                if (selectedSpecies == SPECIES_INVALID) {
                    txLine = "SLEEP ERROR\r\n";
                    state = 15;
                } else {
                    i++;
                    state = 6;
                }
            } else if ((unsigned char)(i - sleepTypeStart) < 7) {
                i++;
            } else {
                txLine = "SLEEP ERROR\r\n";
                state = 15;
            }
            break;

        case 6:
            if (isDigit(line[i])) {
                selectedNumber = (unsigned char)(selectedNumber * 10 + (unsigned char)(line[i] - '0'));
                i++;
            } else if (line[i] == '\0' && selectedNumber > 0) {
                Farm_RequestSelectAnimal(selectedSpecies, selectedNumber);
                state = 7;
            } else {
                txLine = "SLEEP ERROR\r\n";
                state = 15;
            }
            break;

        case 7:
            if (Farm_IsSearchFinished() == 1) {
                if (Farm_IsAnimalFound() == 1) {
                    state = 8;
                } else {
                    txLine = "SLEEP ERROR\r\n";
                    state = 15;
                }
            }
            break;

        case 8:
            if (Farm_IsRestFinished() == 1) {
                if (Farm_IsRestSuccess() == 1) {
                    txLine = "SLEEP OK\r\n";
                } else {
                    txLine = "SLEEP ERROR\r\n";
                }
                state = 15;
            }
            break;

        case 11:
            buildProductsLine();
            txLine = txBuffer;
            state = 15;
            break;

        case 12:
            if (animalIndex < Farm_GetAnimalCount()) {
                buildAnimalLine(animalIndex);
                txLine = txBuffer;
                state = 17;
            } else {
                txLine = "F\r\n";
                state = 15;
            }
            break;

        case 13:
            if (recipeId <= 3) {
                Farm_Consume(recipeId);
                txLine = "CONSUME OK\r\n";
            } else {
                txLine = "CONSUME ERROR\r\n";
            }
            state = 15;
            break;

        case 14:
            txLine = "INIT ERROR\r\n";
            state = 15;
            break;

        case 15:
            if (SJ_PutString(txLine)) {
                state = 0;
            }
            break;

        case 16:
            if (joystickEvent == JOY_EVT_UP) {
                txLine = "U\r\n";
                state = 15;
            } else if (joystickEvent == JOY_EVT_DOWN) {
                txLine = "D\r\n";
                state = 15;
            } else if (joystickEvent == JOY_EVT_LEFT) {
                txLine = "L\r\n";
                state = 15;
            } else if (joystickEvent == JOY_EVT_RIGHT) {
                txLine = "R\r\n";
                state = 15;
            } else {
                state = 0;
            }
            break;

        case 17:
            if (SJ_PutString(txLine)) {
                animalIndex++;
                state = 12;
            }
            break;
    }
}
