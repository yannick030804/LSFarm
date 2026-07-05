#include <xc.h>
#include "TAD_EEPROM.h"

unsigned char eepromMode;
static unsigned char clearIndex;

static void launchWrite (void) {
    EECON1bits.EEPGD = 0;
    EECON1bits.CFGS = 0;
    EECON1bits.WREN = 1;

    di();
    EECON2 = 0x55;
    EECON2 = 0xAA;
    EECON1bits.WR = 1;
    ei();
}

void motorEEPROM (void) {
    if (eepromMode == 0) {
        return;
    }

    if (EECON1bits.WR == 1) {
        return;
    }

    if (eepromMode == 2) {
        EEADR = clearIndex;
        EEDATA = 0;
        launchWrite();
        clearIndex++;
        if (clearIndex == 0) {
            eepromMode = 3;
        }
    } else if (eepromMode == 1) {
        launchWrite();
        eepromMode = 3;
    } else {
        EECON1bits.WREN = 0;
        eepromMode = 0;
    }
}

unsigned char EEPROM_ReadByte (unsigned char addr) {
    EEADR = addr;
    EECON1bits.EEPGD = 0;
    EECON1bits.CFGS = 0;
    EECON1bits.RD = 1;
    return EEDATA;
}

unsigned char EEPROM_StartByteWrite (unsigned char addr, unsigned char data) {
    if (eepromMode != 0) {
        return 0;
    }

    EEADR = addr;
    EEDATA = data;
    eepromMode = 1;
    return 1;
}

void EEPROM_RequestClear (void) {
    if (eepromMode != 0) {
        return;
    }

    clearIndex = 0;
    eepromMode = 2;
}
