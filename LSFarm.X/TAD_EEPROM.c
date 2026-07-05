#include <xc.h>
#include "TAD_EEPROM.h"

unsigned char EEPROM_Mode;
static unsigned char clearIndex;

#define mode EEPROM_Mode

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
    if (mode == 0) {
        return;
    }

    if (EECON1bits.WR == 1) {
        return;
    }

    if (mode == 2) {
        EEADR = clearIndex;
        EEDATA = 0;
        launchWrite();
        clearIndex++;
        if (clearIndex == 0) {
            mode = 3;
        }
    } else if (mode == 1) {
        launchWrite();
        mode = 3;
    } else {
        EECON1bits.WREN = 0;
        mode = 0;
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
    if (mode != 0) {
        return 0;
    }

    EEADR = addr;
    EEDATA = data;
    mode = 1;
    return 1;
}

void EEPROM_RequestClear (void) {
    if (mode != 0) {
        return;
    }

    clearIndex = 0;
    mode = 2;
}
