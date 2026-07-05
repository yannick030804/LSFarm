#include <xc.h>
#include "TAD_DISPLAY.h"
#include "TAD_FARM.h"
#include "TAD_LCD.h"
#include "TAD_SERIAL_TIME.h"
#include "TAD_TIMER.h"

#define DISPLAY_MESSAGE_TIME 3000UL
#define DISPLAY_TIMER_MASK 0x07
#define DISPLAY_STATE_SHIFT 3
#define DISPLAY_TIMER_HANDLE() ((unsigned char)(displayCtrl & DISPLAY_TIMER_MASK))
#define DISPLAY_GET_STATE() ((unsigned char)(displayCtrl >> DISPLAY_STATE_SHIFT))
#define DISPLAY_SET_STATE(value) (displayCtrl = (char)((displayCtrl & DISPLAY_TIMER_MASK) | ((value) << DISPLAY_STATE_SHIFT)))

static char displayLine[17];
static char displayCtrl;

static void Display_LoadMessage (unsigned char messageId) {
    if (messageId < 2) {
        displayLine[0] = 'N';
        displayLine[1] = 'u';
        displayLine[2] = 'e';
        displayLine[3] = 'v';
        displayLine[4] = 'o';
        displayLine[5] = ' ';
        if (messageId == 0) {
            displayLine[6] = 'A';
            displayLine[7] = 'n';
            displayLine[8] = 'i';
            displayLine[9] = 'm';
            displayLine[10] = 'a';
            displayLine[11] = 'l';
            displayLine[12] = '\0';
        } else {
            displayLine[6] = 'P';
            displayLine[7] = 'r';
            displayLine[8] = 'o';
            displayLine[9] = 'd';
            displayLine[10] = 'u';
            displayLine[11] = 'c';
            displayLine[12] = 't';
            displayLine[13] = 'o';
            displayLine[14] = '\0';
        }
    } else if (messageId == 2) {
        displayLine[0] = 'E';
        displayLine[1] = 's';
        displayLine[2] = 'p';
        displayLine[3] = 'e';
        displayLine[4] = 'r';
        displayLine[5] = 'a';
        displayLine[6] = 'n';
        displayLine[7] = 'd';
        displayLine[8] = 'o';
        displayLine[9] = ' ';
        displayLine[10] = 'I';
        displayLine[11] = 'n';
        displayLine[12] = 'i';
        displayLine[13] = 't';
        displayLine[14] = '\0';
    } else if (messageId == 3) {
        displayLine[0] = 'F';
        displayLine[1] = 'a';
        displayLine[2] = 'l';
        displayLine[3] = 't';
        displayLine[4] = 'a';
        displayLine[5] = ' ';
        displayLine[6] = 'H';
        displayLine[7] = 'o';
        displayLine[8] = 'r';
        displayLine[9] = 'a';
        displayLine[10] = '\0';
    } else {
        displayLine[0] = '\0';
    }
}

static void Display_Put2Digits (unsigned char index, unsigned char value) {
    unsigned char digit = 0;

    while (value >= 10) {
        value = (unsigned char)(value - 10);
        digit++;
    }

    displayLine[index] = (char)('0' + digit);
    displayLine[index + 1] = (char)('0' + value);
}

static void Display_PutFixedLine (unsigned char row, const char *text) {
    unsigned char i = 0;

    LCD_GotoXY(0, row);
    while (text[i] != '\0' && i < 16) {
        LCD_PutChar(text[i]);
        i++;
    }
    while (i < 16) {
        LCD_PutChar(' ');
        i++;
    }
}

static void Display_PutNumber (unsigned char value) {
    unsigned char index = 0;
    unsigned char digit = 0;

    if (value >= 100) {
        while (value >= 100) {
            value = (unsigned char)(value - 100);
            digit++;
        }
        displayLine[index++] = (char)('0' + digit);
        digit = 0;
        while (value >= 10) {
            value = (unsigned char)(value - 10);
            digit++;
        }
        displayLine[index++] = (char)('0' + digit);
    } else if (value >= 10) {
        while (value >= 10) {
            value = (unsigned char)(value - 10);
            digit++;
        }
        displayLine[index++] = (char)('0' + digit);
    }
    displayLine[index++] = (char)('0' + value);
    displayLine[index] = '\0';
}

static void Display_ShowNotification (const FarmNotification *notification) {
    if (notification->kind == FARM_NOTIFICATION_ANIMAL) {
        Display_LoadMessage(0);
    } else {
        Display_LoadMessage(1);
    }
    Display_PutFixedLine(0, displayLine);
    Display_PutNumber(notification->number);
    Display_PutFixedLine(1, displayLine);
}

static void Display_ShowIdleScreen (void) {
    if (Farm_IsConfigured() == 0) {
        Display_LoadMessage(2);
        Display_PutFixedLine(0, displayLine);
        displayLine[0] = '\0';
        Display_PutFixedLine(1, displayLine);
        return;
    }

    if (SerialTime_IsConfigured() == 0) {
        Display_PutFixedLine(0, Farm_GetName());
        Display_LoadMessage(3);
        Display_PutFixedLine(1, displayLine);
        return;
    }

    Display_Put2Digits(0, SerialTime_GetDay());
    displayLine[2] = '/';
    Display_Put2Digits(3, SerialTime_GetMonth());
    displayLine[5] = '\0';
    Display_PutFixedLine(0, Farm_GetName());
    Display_PutFixedLine(1, displayLine);
}

void Display_Init (void) {
    TI_NewTimer((unsigned char *)&displayCtrl);
}

void motorDisplay (void) {
    FarmNotification notification;

    switch (DISPLAY_GET_STATE()) {
        case 0:
            if (Farm_GetNotification(&notification) == 1) {
                Display_ShowNotification(&notification);
                TI_ResetTics(DISPLAY_TIMER_HANDLE());
                DISPLAY_SET_STATE(DISPLAY_GET_STATE() + 1);
            } else {
                Display_ShowIdleScreen();
            }
            break;
        case 1:
            if (TI_GetTics(DISPLAY_TIMER_HANDLE()) >= DISPLAY_MESSAGE_TIME) {
                DISPLAY_SET_STATE(DISPLAY_GET_STATE() - 1);
            }
            break;
    }
}
