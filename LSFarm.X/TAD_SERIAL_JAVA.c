#include <xc.h>
#include "TAD_SERIAL_JAVA.h"

static unsigned char stringIndex;
static char line[32];
static unsigned char lineIndex;

void SerialJava_Init (void) {
    CONFIG_SERIAL_JAVA;

    TXSTA = 0x24;
    RCSTA = 0x90;
    SPBRG = 64;
    BAUDCON = 0x00;

    stringIndex = 0;
    lineIndex = 0;
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

const char *SJ_GetLine(void) {
    unsigned char c;

    if (!PIR1bits.RCIF) {
        return 0;
    }

    c = RCREG;

    if (c == '\r' || c == '\n') {
        if (lineIndex == 0) {
            return 0;
        }

        line[lineIndex] = '\0';
        lineIndex = 0;
        return line;
    }

    if (lineIndex < sizeof(line) - 1) {
        line[lineIndex] = c;
        lineIndex++;
    }

    return 0;
}