#include <xc.h>
#include "TAD_SERIAL_TIME.h"

static volatile unsigned char rxByte;
static volatile unsigned char rxReady;
static volatile unsigned char rxActive;
static volatile unsigned char txActive;
static unsigned char rxLen;
static unsigned char dateReady;
static STDate currentDate;

void SerialTime_Init(void) {
    CONFIG_SERIAL_TIME;

    rxByte = 0;
    rxReady = 0;
    rxActive = 0;
    txActive = 0;
    rxLen = 0;
    dateReady = 0;

    currentDate.day = 0;
    currentDate.month = 0;
    currentDate.hour = 0;
    currentDate.minute = 0;
    currentDate.second = 0;

    INTCON2bits.INTEDG2 = 0;
    INTCON3bits.INT2IF = 0;
    INTCON3bits.INT2IE = 1;
}

void motorSerialTime (void) {
    static unsigned char state = 0;

    switch (state) {
        
    }
}
