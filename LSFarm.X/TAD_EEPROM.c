#include <xc.h>
#include "TAD_EEPROM.h"

static unsigned char mode;
static unsigned int clearIndex;
static unsigned char byteAddr;
static unsigned char byteData;

static void launchWrite (unsigned char addr, unsigned char data) {
    EEADR = addr;
    EEDATA = data;
    EECON1bits.EEPGD = 0;
    EECON1bits.CFGS = 0;
    EECON1bits.WREN = 1;

    di();
    EECON2 = 0x55;
    EECON2 = 0xAA;
    EECON1bits.WR = 1;
    ei();
}

void EEPROM_Init (void) {
    mode = 0;
    clearIndex = 0;
    byteAddr = 0;
    byteData = 0;
}

void motorEEPROM (void) {
    static unsigned char state = 0;

    switch (state) {
        case 0:
            if (mode != 0) {
                state = 1;
            }
            break;
        case 1:
            if (EECON1bits.WR == 1) {
                break;
            }

            if (mode == 2) {
                if (clearIndex >= 256U) {
                    EECON1bits.WREN = 0;
                    mode = 0;
                    state = 0;
                } else {
                    launchWrite((unsigned char)clearIndex, 0);
                    clearIndex++;
                }
            } else if (mode == 1) {
                launchWrite(byteAddr, byteData);
                mode = 4;
            } else if (mode == 4) {
                EECON1bits.WREN = 0;
                mode = 0;
                state = 0;
            } else {
                EECON1bits.WREN = 0;
                mode = 0;
                state = 0;
            }
            break;
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

    byteAddr = addr;
    byteData = data;
    mode = 1;
    return 1;
}

unsigned char EEPROM_IsBusy (void) {
    if (mode != 0) {
        return 1;
    }
    return 0;
}

void EEPROM_RequestClear (void) {
    if (mode != 0) {
        return;
    }

    clearIndex = 0;
    mode = 2;
}
