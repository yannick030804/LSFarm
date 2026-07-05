#include <xc.h>
#include "TAD_SERIAL_TIME.h"
#include "TAD_TIMER.h"

#define SERIAL_TIME_LINE_MAX 14
#define ST_FLAG_RX_READY 0x01
#define ST_FLAG_RX_ACTIVE 0x02
#define ST_FLAG_TX_ACTIVE 0x04

static unsigned char timerHandle;

static volatile unsigned char rxByte;
static volatile unsigned char stFlags;
static volatile unsigned char rxBitIdx;
static volatile unsigned char txBitIdx;
static volatile unsigned char rxShift;
static volatile unsigned char rxPos;
static volatile unsigned char rxNext;

static volatile unsigned char txShift;
static volatile unsigned char txPos;
static volatile unsigned char txNext;
static volatile unsigned char txEcho;
static const char *txPtr;

static char rxChars[SERIAL_TIME_LINE_MAX];
static unsigned char rxLen;
unsigned char serialTimeConfigured;
unsigned char serialTimeDay;
unsigned char serialTimeMonth;
unsigned char serialTimeHour;
unsigned char serialTimeMinute;
unsigned char serialTimeSecond;

static unsigned char isDigit (char c) {
    if (c >= '0' && c <= '9') {
        return 1;
    }
    return 0;
}

static unsigned char parseTwoDigits (unsigned char index) {
    unsigned char value = (unsigned char)(rxChars[index] - '0');
    return (unsigned char)((unsigned char)((value << 3) + (value << 1)) + (unsigned char)(rxChars[index + 1] - '0'));
}

static unsigned char SerialTime_ParseLine (void) {
    unsigned char day;
    unsigned char month;
    unsigned char hour;
    unsigned char minute;
    unsigned char second;

    if (rxLen != SERIAL_TIME_LINE_MAX) {
        return 0;
    }

    if (rxChars[2] != '/' || rxChars[5] != ' ' || rxChars[8] != ':' || rxChars[11] != ':') {
        return 0;
    }

    if (isDigit(rxChars[0]) == 0 || isDigit(rxChars[1]) == 0 ||
        isDigit(rxChars[3]) == 0 || isDigit(rxChars[4]) == 0 ||
        isDigit(rxChars[6]) == 0 || isDigit(rxChars[7]) == 0 ||
        isDigit(rxChars[9]) == 0 || isDigit(rxChars[10]) == 0 ||
        isDigit(rxChars[12]) == 0 || isDigit(rxChars[13]) == 0) {
        return 0;
    }

    day = parseTwoDigits(0);
    month = parseTwoDigits(3);
    hour = parseTwoDigits(6);
    minute = parseTwoDigits(9);
    second = parseTwoDigits(12);

    if (day < 1 || day > 31 || month < 1 || month > 12 || hour > 23 || minute > 59 || second > 59) {
        return 0;
    }

    serialTimeDay = day;
    serialTimeMonth = month;
    serialTimeHour = hour;
    serialTimeMinute = minute;
    serialTimeSecond = second;
    serialTimeConfigured = 1;
    return 1;
}

static void txTick (void) {
    unsigned char b;

    if ((stFlags & ST_FLAG_TX_ACTIVE) == 0) {
        b = 0;

        if (txEcho != 0) {
            b = txEcho;
            txEcho = 0;
        } else if (txPtr != 0) {
            if (*txPtr != '\0') {
                b = *txPtr;
                txPtr++;
            } else {
                txPtr = 0;
            }
        }

        if (b != 0) {
            txShift = b;
            txBitIdx = 0;
            txPos = 0;
            txNext = 3;
            stFlags |= ST_FLAG_TX_ACTIVE;
        }
        return;
    }

    txPos = (unsigned char)(txPos + 3);
    if (txPos < txNext) {
        return;
    }
    txNext = (unsigned char)(txNext + 10);

    if (txBitIdx == 0) {
        LATBbits.LATB1 = 0;
    } else if (txBitIdx <= 8) {
        LATBbits.LATB1 = txShift & 1;
        txShift >>= 1;
    } else if (txBitIdx == 9) {
        LATBbits.LATB1 = 1;
    } else {
        stFlags &= (unsigned char)(~ST_FLAG_TX_ACTIVE);
        return;
    }

    txBitIdx++;
}

void SerialTime_Init (void) {
    CONFIG_SERIAL_TIME;

    INTCON2bits.INTEDG2 = 0;
    INTCON3bits.INT2IF = 0;
    INTCON3bits.INT2IE = 1;

    TI_NewTimer(&timerHandle);
}

void motorSerialTime (void) {
    static unsigned char state = 0;
    unsigned char c;

    switch (state) {
        case 0:
            if ((stFlags & ST_FLAG_RX_READY) != 0) {
                c = rxByte;
                stFlags &= (unsigned char)(~ST_FLAG_RX_READY);

                if (c == '\r' || c == '\n') {
                    if (rxLen > 0) {
                        state = 1;
                    }
                } else if (rxLen < SERIAL_TIME_LINE_MAX) {
                    txEcho = c;
                    rxChars[rxLen] = (char)c;
                    rxLen++;
                } else {
                    rxLen = 0;
                }
            }
            break;
        case 1:
            if (SerialTime_ParseLine() == 1) {
                txPtr = "\r\nDate and time correct\r\n";
            } else {
                txPtr = "\r\nPlease input a correct date\r\n";
            }
            rxLen = 0;
            state = 2;
            TI_ResetTics(timerHandle);
            break;
        case 2:
            if (TI_GetTics(timerHandle) >= 1000) {
                TI_ResetTics(timerHandle);
                serialTimeSecond++;
                if (serialTimeSecond > 59) {
                    serialTimeSecond = 0;
                    serialTimeMinute++;
                    if (serialTimeMinute > 59) {
                        serialTimeMinute = 0;
                        serialTimeHour++;
                        if (serialTimeHour > 23) {
                            serialTimeHour = 0;
                            serialTimeDay++;
                            if (serialTimeDay > 31) {
                                serialTimeDay = 1;
                                serialTimeMonth++;
                                if (serialTimeMonth > 12) {
                                    serialTimeMonth = 1;
                                }
                            }
                        }
                    }
                }
            }
            break;
    }
}

void SerialTime_StartBitISR (void) {
    INTCON3bits.INT2IE = 0;
    rxShift = 0;
    rxBitIdx = 0;
    rxPos = 0;
    rxNext = 15;
    stFlags |= ST_FLAG_RX_ACTIVE;
}

void SerialTime_TickISR (void) {
    if ((stFlags & ST_FLAG_RX_ACTIVE) == 0) {
        txTick();
        return;
    }

    rxPos = (unsigned char)(rxPos + 3);
    if (rxPos < rxNext) {
        txTick();
        return;
    }
    rxNext = (unsigned char)(rxNext + 10);

    if (rxBitIdx < 8) {
        rxShift >>= 1;
        if (PORTBbits.RB2) {
            rxShift |= 0x80;
        }
        rxBitIdx++;
    } else {
        if (PORTBbits.RB2) {
            rxByte = rxShift;
            stFlags |= ST_FLAG_RX_READY;
        }
        stFlags &= (unsigned char)(~ST_FLAG_RX_ACTIVE);
        INTCON3bits.INT2IF = 0;
        INTCON3bits.INT2IE = 1;
    }

    txTick();
}
