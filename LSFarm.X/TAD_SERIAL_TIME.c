#include <xc.h>
#include "TAD_SERIAL_TIME.h"
#include "TAD_TIMER.h"

#define SERIAL_TIME_LINE_MAX 14

static unsigned char timerHandle;

static volatile unsigned char rxByte;
static volatile unsigned char rxReady;
static volatile unsigned char rxActive;
static volatile unsigned char rxBitIdx;
static volatile unsigned char rxShift;
static volatile unsigned char rxPos;
static volatile unsigned char rxNext;

static volatile unsigned char txActive;
static volatile unsigned char txBitIdx;
static volatile unsigned char txShift;
static volatile unsigned char txPos;
static volatile unsigned char txNext;
static volatile unsigned char txEcho;
static const char *txPtr;

static char rxLine[SERIAL_TIME_LINE_MAX + 1];
static unsigned char rxLen;
static unsigned char timeConfigured;
static STDate currentDate;

static unsigned char isDigit (char c) {
    if (c >= '0' && c <= '9') {
        return 1;
    }
    return 0;
}

static unsigned char parseTwoDigits (unsigned char index) {
    return (unsigned char)((rxLine[index] - '0') * 10 + (rxLine[index + 1] - '0'));
}

static void txTick (void) {
    unsigned char b;

    if (txActive == 0) {
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
            txActive = 1;
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
        txActive = 0;
        return;
    }

    txBitIdx++;
}

void SerialTime_Init (void) {
    CONFIG_SERIAL_TIME;

    rxByte = 0;
    rxReady = 0;
    rxActive = 0;
    rxBitIdx = 0;
    rxShift = 0;
    rxPos = 0;
    rxNext = 0;
    txActive = 0;
    txBitIdx = 0;
    txShift = 0;
    txPos = 0;
    txNext = 0;
    txEcho = 0;
    txPtr = 0;

    rxLen = 0;
    timeConfigured = 0;

    currentDate.day = 0;
    currentDate.month = 0;
    currentDate.hour = 0;
    currentDate.minute = 0;
    currentDate.second = 0;

    INTCON2bits.INTEDG2 = 0;
    INTCON3bits.INT2IF = 0;
    INTCON3bits.INT2IE = 1;

    TI_NewTimer(&timerHandle);
}

void motorSerialTime (void) {
    static unsigned char state = 0;
    static STDate tempDate;
    unsigned char c;

    switch (state) {
        case 0:
            if (rxReady == 1) {
                c = rxByte;
                rxReady = 0;

                if (c == '\r' || c == '\n') {
                    if (rxLen > 0) {
                        rxLine[rxLen] = '\0';
                        state = 1;
                    }
                } else if (rxLen < SERIAL_TIME_LINE_MAX) {
                    txEcho = c;
                    rxLine[rxLen] = c;
                    rxLen++;
                } else {
                    rxLen = 0;
                }
            }
            break;
        case 1:
            tempDate.day = 0;
            tempDate.month = 0;
            tempDate.hour = 0;
            tempDate.minute = 0;
            tempDate.second = 0;
            if (rxLen == SERIAL_TIME_LINE_MAX) {
                state = 2;
            } else {
                txPtr = "\r\nPlease input a correct date\r\n";
                rxLen = 0;
                state = 0;
            }
            break;
        case 2:
            if (isDigit(rxLine[0]) && isDigit(rxLine[1])) {
                tempDate.day = parseTwoDigits(0);
                state = 3;
            } else {
                txPtr = "\r\nPlease input a correct date\r\n";
                rxLen = 0;
                state = 0;
            }
            break;
        case 3:
            if (rxLine[2] == '/') {
                state = 4;
            } else {
                txPtr = "\r\nPlease input a correct date\r\n";
                rxLen = 0;
                state = 0;
            }
            break;
        case 4:
            if (isDigit(rxLine[3]) && isDigit(rxLine[4])) {
                tempDate.month = parseTwoDigits(3);
                state = 5;
            } else {
                txPtr = "\r\nPlease input a correct date\r\n";
                rxLen = 0;
                state = 0;
            }
            break;
        case 5:
            if (rxLine[5] == ' ') {
                state = 6;
            } else {
                txPtr = "\r\nPlease input a correct date\r\n";
                rxLen = 0;
                state = 0;
            }
            break;
        case 6:
            if (isDigit(rxLine[6]) && isDigit(rxLine[7])) {
                tempDate.hour = parseTwoDigits(6);
                state = 7;
            } else {
                txPtr = "\r\nPlease input a correct date\r\n";
                rxLen = 0;
                state = 0;
            }
            break;
        case 7:
            if (rxLine[8] == ':') {
                state = 8;
            } else {
                txPtr = "\r\nPlease input a correct date\r\n";
                rxLen = 0;
                state = 0;
            }
            break;
        case 8:
            if (isDigit(rxLine[9]) && isDigit(rxLine[10])) {
                tempDate.minute = parseTwoDigits(9);
                state = 9;
            } else {
                txPtr = "\r\nPlease input a correct date\r\n";
                rxLen = 0;
                state = 0;
            }
            break;
        case 9:
            if (rxLine[11] == ':') {
                state = 10;
            } else {
                txPtr = "\r\nPlease input a correct date\r\n";
                rxLen = 0;
                state = 0;
            }
            break;
        case 10:
            if (isDigit(rxLine[12]) && isDigit(rxLine[13])) {
                tempDate.second = parseTwoDigits(12);
                state = 11;
            } else {
                txPtr = "\r\nPlease input a correct date\r\n";
                rxLen = 0;
                state = 0;
            }
            break;
        case 11:
            if (tempDate.day >= 1 && tempDate.day <= 31 && tempDate.month >= 1 && tempDate.month <= 12 && tempDate.hour <= 23 && tempDate.minute <= 59 && tempDate.second <= 59) {
                currentDate = tempDate;
                timeConfigured = 1;
                txPtr = "\r\nDate and time correct\r\n";
            } else {
                txPtr = "\r\nPlease input a correct date\r\n";
            }
            rxLen = 0;
            state = 12;
            TI_ResetTics(timerHandle);
            break;
        case 12:
            if (TI_GetTics(timerHandle) >= 1000) {
                TI_ResetTics(timerHandle);
                currentDate.second++;
                if (currentDate.second > 59) {
                    currentDate.second = 0;
                    currentDate.minute++;
                    if (currentDate.minute > 59) {
                        currentDate.minute = 0;
                        currentDate.hour++;
                        if (currentDate.hour > 23) {
                            currentDate.hour = 0;
                            currentDate.day++;
                            if (currentDate.day > 31) {
                                currentDate.day = 1;
                                currentDate.month++;
                                if (currentDate.month > 12) {
                                    currentDate.month = 1;
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
    rxActive = 1;
}

void SerialTime_TickISR (void) {
    if (rxActive == 0) {
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
            rxReady = 1;
        }
        rxActive = 0;
        INTCON3bits.INT2IF = 0;
        INTCON3bits.INT2IE = 1;
    }

    txTick();
}

unsigned char SerialTime_IsConfigured (void) {
    return timeConfigured;
}

const STDate *SerialTime_GetDate (void) {
    if (timeConfigured == 0) {
        return 0;
    }
    return &currentDate;
}

unsigned char SerialTime_GetDay (void) {
    return currentDate.day;
}

unsigned char SerialTime_GetMonth (void) {
    return currentDate.month;
}

unsigned char SerialTime_GetHour (void) {
    return currentDate.hour;
}

unsigned char SerialTime_GetMinute (void) {
    return currentDate.minute;
}

unsigned char SerialTime_GetSecond (void) {
    return currentDate.second;
}
