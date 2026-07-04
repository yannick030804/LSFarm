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
#define CTRL_FLAG_INIT_SENT         0x01
#define CTRL_FLAG_PERSISTENCE_LOADED 0x02
#define CTRL_FLAG_RESET_PENDING     0x04
#define CTRL_FLAG_SAVE_ACTIVE       0x08

static const char *line;
static const char *txLine;
static char farmName[17];
static unsigned char farmNameIndex;
static unsigned char param1;
static unsigned char param2;
static unsigned char param3;
static unsigned char param4;
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
    param1 = 0;
    param2 = 0;
    param3 = 0;
    param4 = 0;
    selectedNumber = 0;
    selectedSpecies = 255;
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

static unsigned char tokenRangeEquals (const char *text, unsigned char start, unsigned char end, const char *token) {
    unsigned char i = start;
    unsigned char j = 0;

    while (i < end && token[j] != '\0') {
        if (text[i] != token[j]) {
            return 0;
        }
        i++;
        j++;
    }

    if (i == end && token[j] == '\0') {
        return 1;
    }
    return 0;
}

static unsigned char getSpeciesFromRange (const char *text, unsigned char start, unsigned char end) {
    if (tokenRangeEquals(text, start, end, "VACA")) {
        return 0;
    }
    if (tokenRangeEquals(text, start, end, "PORC")) {
        return 1;
    }
    if (tokenRangeEquals(text, start, end, "CAVALL")) {
        return 2;
    }
    if (tokenRangeEquals(text, start, end, "GALLINA")) {
        return 3;
    }
    return 255;
}

static unsigned char appendNum (unsigned char index, unsigned char value) {
    unsigned char d;
    unsigned char started = 0;

    d = value / 100;
    if (d != 0) {
        txBuffer[index] = (char)('0' + d);
        index++;
        started = 1;
    }

    d = (unsigned char)((value / 10) % 10);
    if (d != 0 || started == 1) {
        txBuffer[index] = (char)('0' + d);
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
    txBuffer[index] = '$';
    index++;
    index = appendNum(index, Farm_GetProductTotal(1));
    txBuffer[index] = '$';
    index++;
    index = appendNum(index, Farm_GetProductTotal(3));
    txBuffer[index] = '$';
    index++;
    index = appendNum(index, Farm_GetProductTotal(2));
    txBuffer[index] = '\r';
    index++;
    txBuffer[index] = '\n';
    index++;
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
        txBuffer[index] = *name;
        index++;
        name++;
    }

    txBuffer[index] = '$';
    index++;
    index = appendNum(index, number);
    txBuffer[index] = '$';
    index++;

    if (critical == 1) {
        name = "SLEEP";
    } else {
        name = "AWAKE";
    }

    while (*name != '\0') {
        txBuffer[index] = *name;
        index++;
        name++;
    }

    txBuffer[index] = '\r';
    index++;
    txBuffer[index] = '\n';
    index++;
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
                state = 20;
            } else {
                joystickEvent = Joystick_GetEvent();
                if (joystickEvent != JOY_EVT_NONE) {
                    state = 21;
                }
            }
            break;

        case 1:
            if (line[0] == 'I' && line[1] == ':') {
                i = 2;
                state = 2;
            } else if (line[0] == 'G' && line[1] == '\0') {
                state = 16;
            } else if (line[0] == 'A' && line[1] == '\0') {
                animalIndex = 0;
                state = 17;
            } else if (line[0] == 'R' && line[1] == '\0') {
                Farm_Reset();
                Heartbeat_SetRebellion(0);
                controllerFlags |= CTRL_FLAG_RESET_PENDING;
                txLine = "RESET OK\r\n";
                state = 20;
            } else if (line[0] == 'B' && line[1] == '\0') {
                Farm_SetRebellion(1);
                Heartbeat_SetRebellion(1);
                txLine = "REBELLION ON\r\n";
                state = 20;
            } else if (line[0] == 'P' && line[1] == '\0') {
                Farm_SetRebellion(0);
                Heartbeat_SetRebellion(0);
                txLine = "REBELLION OFF\r\n";
                state = 20;
            } else if (line[0] == 'C' && line[1] == ':' && isDigit(line[2]) && line[3] == '\0') {
                recipeId = (unsigned char)(line[2] - '0');
                state = 19;
            } else if (line[0] == 'S' && line[1] == ':') {
                selectedNumber = 0;
                selectedSpecies = 255;
                i = 2;
                sleepTypeStart = 2;
                state = 9;
            } else {
                state = 8;
            }
            break;
        case 2:
            if (line[i] == '\0') {
                state = 8;
            } else if (line[i] == '$') {
                farmName[farmNameIndex] = '\0';
                i++;
                state = 3;
            } else {
                if (farmNameIndex < sizeof(farmName) - 1) {
                    farmName[farmNameIndex] = line[i];
                    farmNameIndex++;
                }
                i++;
            }
            break;
        case 3:
            if (isDigit(line[i])) {
                param1 = param1 * 10 + (line[i] - '0');
                i++;
            } else if (line[i] == '$') {
                i++;
                state = 4;
            } else {
                state = 8;
            }
            break;
        case 4:
            if (isDigit(line[i])) {
                param2 = param2 * 10 + (line[i] - '0');
                i++;
            } else if (line[i] == '$') {
                i++;
                state = 5;
            } else {
                state = 8;
            }
            break;
        case 5:
            if (isDigit(line[i])) {
                param3 = param3 * 10 + (line[i] - '0');
                i++;
            } else if (line[i] == '$') {
                i++;
                state = 6;
            } else {
                state = 8;
            }
            break;
        case 6:
            if (isDigit(line[i])) {
                param4 = param4 * 10 + (line[i] - '0');
                i++;
            } else if (line[i] == '\0') {
                state = 7;
            } else {
                state = 8;
            }
            break;
        case 7:
            if (farmName[0] == '\0' || isValidGenerationTime(param1) == 0 || isValidGenerationTime(param2) == 0 || isValidGenerationTime(param3) == 0 || isValidGenerationTime(param4) == 0) {
                state = 8;
                break;
            }
            if ((controllerFlags & CTRL_FLAG_INIT_SENT) == 0) {
                Farm_RequestConfigure(farmName, param1, param2, param3, param4);
                controllerFlags |= CTRL_FLAG_INIT_SENT;
            }
            if (Farm_IsConfigured() == 1) {
                txLine = "INIT OK\r\n";
                state = 20;
            }
            break;

        case 8:
            txLine = "INIT ERROR\r\n";
            state = 20;
            break;
        case 9:
            if (line[i] == '\0') {
                state = 15;
            } else if (line[i] == '$') {
                selectedSpecies = getSpeciesFromRange(line, sleepTypeStart, i);
                if (selectedSpecies == 255) {
                    state = 15;
                } else {
                    i++;
                    state = 10;
                }
            } else if (i - sleepTypeStart < 7) {
                i++;
            } else {
                state = 15;
            }
            break;
        case 10:
            if (isDigit(line[i])) {
                selectedNumber = selectedNumber * 10 + (line[i] - '0');
                i++;
            } else if (line[i] == '\0' && selectedNumber > 0) {
                Farm_RequestSelectAnimal(selectedSpecies, selectedNumber);
                state = 11;
            } else {
                state = 15;
            }
            break;
        case 11:
            if (Farm_IsSearchFinished() == 1) {
                if (Farm_IsAnimalFound() == 1) {
                    state = 13;
                } else {
                    state = 15;
                }
            }
            break;
        case 13:
            if (Farm_IsRestFinished() == 1) {
                if (Farm_IsRestSuccess() == 1) {
                    state = 14;
                } else {
                    state = 15;
                }
            }
            break;
        case 14:
            txLine = "SLEEP OK\r\n";
            state = 20;
            break;
        case 15:
            txLine = "SLEEP ERROR\r\n";
            state = 20;
            break;
        case 16:
            buildProductsLine();
            txLine = txBuffer;
            state = 20;
            break;
        case 17:
            if (animalIndex < Farm_GetAnimalCount()) {
                buildAnimalLine(animalIndex);
                txLine = txBuffer;
                state = 18;
            } else {
                txLine = "F\r\n";
                state = 20;
            }
            break;
        case 18:
            if (SJ_PutString(txLine)) {
                animalIndex++;
                state = 17;
            }
            break;
        case 19:
            if (recipeId <= 3) {
                Farm_Consume(recipeId);
                txLine = "CONSUME OK\r\n";
            } else {
                txLine = "CONSUME ERROR\r\n";
            }
            state = 20;
            break;
        case 20:
            if (SJ_PutString(txLine)) {
                state = 0;
            }
            break;
        case 21:
            if (joystickEvent == JOY_EVT_UP) {
                txLine = "U\r\n";
            } else if (joystickEvent == JOY_EVT_DOWN) {
                txLine = "D\r\n";
            } else if (joystickEvent == JOY_EVT_LEFT) {
                txLine = "L\r\n";
            } else if (joystickEvent == JOY_EVT_RIGHT) {
                txLine = "R\r\n";
            } else {
                state = 0;
                break;
            }
            state = 20;
            break;
    }
}
