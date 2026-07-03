#include <xc.h>
#include "TAD_EEPROM.h"

static unsigned char buffer[16];
static unsigned char writeAddr;
static unsigned char writeLen;
static unsigned char writeIndex;
static unsigned char busy;
static unsigned char mode;
static unsigned int clearIndex;
static unsigned char imageAddr;
static unsigned char imageLen;
static unsigned char imageIndex;
static const unsigned char *imageSrc;

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
    busy = 0;
    mode = 0;
    writeAddr = 0;
    writeLen = 0;
    writeIndex = 0;
    clearIndex = 0;
    imageAddr = 0;
    imageLen = 0;
    imageIndex = 0;
    imageSrc = 0;
}

void motorEEPROM (void) {
    static unsigned char state = 0;

    switch (state) {
        case 0:
            if (busy == 1) {
                state = 1;
            }
            break;
        case 1:
            if (EECON1bits.WR == 1) {
                break;
            }

            if (mode == 1) {
                if (writeIndex >= writeLen) {
                    EECON1bits.WREN = 0;
                    busy = 0;
                    mode = 0;
                    state = 0;
                } else {
                    launchWrite((unsigned char)(writeAddr + writeIndex), buffer[writeIndex]);
                    writeIndex++;
                }
            } else if (mode == 2) {
                if (clearIndex >= 256U) {
                    EECON1bits.WREN = 0;
                    busy = 0;
                    mode = 0;
                    state = 0;
                } else {
                    launchWrite((unsigned char)clearIndex, 0);
                    clearIndex++;
                }
            } else if (mode == 3) {
                if (imageIndex >= imageLen) {
                    EECON1bits.WREN = 0;
                    busy = 0;
                    mode = 0;
                    state = 0;
                } else {
                    launchWrite((unsigned char)(imageAddr + imageIndex), imageSrc[imageIndex]);
                    imageIndex++;
                }
            } else {
                EECON1bits.WREN = 0;
                busy = 0;
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

void EEPROM_ReadBlock (unsigned char addr, unsigned char *dst, unsigned char len) {
    unsigned char i;

    for (i = 0; i < len; i++) {
        dst[i] = EEPROM_ReadByte((unsigned char)(addr + i));
    }
}

unsigned char EEPROM_StartWrite (unsigned char addr, const unsigned char *src, unsigned char len) {
    unsigned char i;

    if (busy == 1 || len > sizeof(buffer)) {
        return 0;
    }

    for (i = 0; i < len; i++) {
        buffer[i] = src[i];
    }

    writeAddr = addr;
    writeLen = len;
    writeIndex = 0;
    mode = 1;
    busy = 1;
    return 1;
}

unsigned char EEPROM_StartImageWrite (unsigned char addr, const unsigned char *src, unsigned char len) {
    if (busy == 1) {
        return 0;
    }

    imageAddr = addr;
    imageLen = len;
    imageIndex = 0;
    imageSrc = src;
    mode = 3;
    busy = 1;
    return 1;
}

unsigned char EEPROM_IsBusy (void) {
    return busy;
}

void EEPROM_RequestClear (void) {
    if (busy == 1) {
        return;
    }

    clearIndex = 0;
    mode = 2;
    busy = 1;
}
