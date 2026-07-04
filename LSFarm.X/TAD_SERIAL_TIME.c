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

static char rxChar0;
static char rxChar1;
static char rxChar2;
static char rxChar3;
static char rxChar4;
static char rxChar5;
static char rxChar6;
static char rxChar7;
static char rxChar8;
static char rxChar9;
static char rxChar10;
static char rxChar11;
static char rxChar12;
static char rxChar13;
static unsigned char rxLen;
static unsigned char timeConfigured;
static STDate currentDate;
static unsigned char tempDay;
static unsigned char tempMonth;
static unsigned char tempHour;
static unsigned char tempMinute;
static unsigned char tempSecond;

static void setRxChar (unsigned char index, char c) {
    switch (index) {
        case 0: rxChar0 = c; break;
        case 1: rxChar1 = c; break;
        case 2: rxChar2 = c; break;
        case 3: rxChar3 = c; break;
        case 4: rxChar4 = c; break;
        case 5: rxChar5 = c; break;
        case 6: rxChar6 = c; break;
        case 7: rxChar7 = c; break;
        case 8: rxChar8 = c; break;
        case 9: rxChar9 = c; break;
        case 10: rxChar10 = c; break;
        case 11: rxChar11 = c; break;
        case 12: rxChar12 = c; break;
        case 13: rxChar13 = c; break;
    }
}

static char getRxChar (unsigned char index) {
    switch (index) {
        case 0: return rxChar0;
        case 1: return rxChar1;
        case 2: return rxChar2;
        case 3: return rxChar3;
        case 4: return rxChar4;
        case 5: return rxChar5;
        case 6: return rxChar6;
        case 7: return rxChar7;
        case 8: return rxChar8;
        case 9: return rxChar9;
        case 10: return rxChar10;
        case 11: return rxChar11;
        case 12: return rxChar12;
        default: return rxChar13;
    }
}

static unsigned char isDigit (char c) {
    if (c >= '0' && c <= '9') {
        return 1;
    }
    return 0;
}

static unsigned char parseTwoDigits (unsigned char index) {
    return (unsigned char)((getRxChar(index) - '0') * 10 + (getRxChar(index + 1) - '0'));
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
    unsigned char c;

    switch (state) {
        case 0:
            if (rxReady == 1) {
                c = rxByte;
                rxReady = 0;

                if (c == '\r' || c == '\n') {
                    if (rxLen > 0) {
                        state = 1;
                    }
                } else if (rxLen < SERIAL_TIME_LINE_MAX) {
                    txEcho = c;
                    setRxChar(rxLen, (char)c);
                    rxLen++;
                } else {
                    rxLen = 0;
                }
            }
            break;
        case 1:
            tempDay = 0;
            tempMonth = 0;
            tempHour = 0;
            tempMinute = 0;
            tempSecond = 0;
            if (rxLen == SERIAL_TIME_LINE_MAX) {
                state = 2;
            } else {
                txPtr = "\r\nPlease input a correct date\r\n";
                rxLen = 0;
                state = 0;
            }
            break;
        case 2:
            if (isDigit(getRxChar(0)) && isDigit(getRxChar(1))) {
                tempDay = parseTwoDigits(0);
                state = 3;
            } else {
                txPtr = "\r\nPlease input a correct date\r\n";
                rxLen = 0;
                state = 0;
            }
            break;
        case 3:
            if (getRxChar(2) == '/') {
                state = 4;
            } else {
                txPtr = "\r\nPlease input a correct date\r\n";
                rxLen = 0;
                state = 0;
            }
            break;
        case 4:
            if (isDigit(getRxChar(3)) && isDigit(getRxChar(4))) {
                tempMonth = parseTwoDigits(3);
                state = 5;
            } else {
                txPtr = "\r\nPlease input a correct date\r\n";
                rxLen = 0;
                state = 0;
            }
            break;
        case 5:
            if (getRxChar(5) == ' ') {
                state = 6;
            } else {
                txPtr = "\r\nPlease input a correct date\r\n";
                rxLen = 0;
                state = 0;
            }
            break;
        case 6:
            if (isDigit(getRxChar(6)) && isDigit(getRxChar(7))) {
                tempHour = parseTwoDigits(6);
                state = 7;
            } else {
                txPtr = "\r\nPlease input a correct date\r\n";
                rxLen = 0;
                state = 0;
            }
            break;
        case 7:
            if (getRxChar(8) == ':') {
                state = 8;
            } else {
                txPtr = "\r\nPlease input a correct date\r\n";
                rxLen = 0;
                state = 0;
            }
            break;
        case 8:
            if (isDigit(getRxChar(9)) && isDigit(getRxChar(10))) {
                tempMinute = parseTwoDigits(9);
                state = 9;
            } else {
                txPtr = "\r\nPlease input a correct date\r\n";
                rxLen = 0;
                state = 0;
            }
            break;
        case 9:
            if (getRxChar(11) == ':') {
                state = 10;
            } else {
                txPtr = "\r\nPlease input a correct date\r\n";
                rxLen = 0;
                state = 0;
            }
            break;
        case 10:
            if (isDigit(getRxChar(12)) && isDigit(getRxChar(13))) {
                tempSecond = parseTwoDigits(12);
                state = 11;
            } else {
                txPtr = "\r\nPlease input a correct date\r\n";
                rxLen = 0;
                state = 0;
            }
            break;
        case 11:
            if (tempDay >= 1 && tempDay <= 31 && tempMonth >= 1 && tempMonth <= 12 && tempHour <= 23 && tempMinute <= 59 && tempSecond <= 59) {
                currentDate.day = tempDay;
                currentDate.month = tempMonth;
                currentDate.hour = tempHour;
                currentDate.minute = tempMinute;
                currentDate.second = tempSecond;
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
