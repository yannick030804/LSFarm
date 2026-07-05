#include <xc.h>
#include "TAD_SERIAL_JAVA.h"

static unsigned char stringIndex;
char serialJavaBuffer[24];
unsigned char serialJavaLineIndex;

void SerialJava_Init (void) {
    CONFIG_SERIAL_JAVA;

    TXSTA = 0x24;
    RCSTA = 0x90;
    SPBRG = 64;
    BAUDCON = 0x00;
}

unsigned char SJ_PutString (const char *str) {
    if (str[stringIndex] == '\0') {
        stringIndex = 0;
        return 1;
    }

    if (PIR1bits.TXIF) {
        TXREG = str[stringIndex];
        stringIndex++;
    }

    return 0;
}

char *SJ_GetLine(void) {
    unsigned char c;

    if (!PIR1bits.RCIF) {
        return 0;
    }

    c = RCREG;

    if (c == '\r' || c == '\n') {
        if (serialJavaLineIndex == 0) {
            return 0;
        }

        serialJavaBuffer[serialJavaLineIndex] = '\0';
        serialJavaLineIndex = 0;
        return serialJavaBuffer;
    }

    if (serialJavaLineIndex < sizeof(serialJavaBuffer) - 1) {
        serialJavaBuffer[serialJavaLineIndex] = c;
        serialJavaLineIndex++;
    }

    return 0;
}
