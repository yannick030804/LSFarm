#include <xc.h>
#include "TAD_SERIAL_JAVA.h"

static unsigned char rxIndex;
static unsigned char txIndex;

void SerialJava_Init(void) {
    CONFIG_SERIAL_JAVA;
    TXSTA = 0x24;
    RCSTA = 0x90;
    SPBRG = 64;
    BAUDCON = 0x00;
    rxIndex = 0;
    txIndex = 0;
}
