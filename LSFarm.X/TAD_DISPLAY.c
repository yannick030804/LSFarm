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

static void Display_ShowText (const char *line1, const char *line2) {
    Display_PutFixedLine(0, line1);
    Display_PutFixedLine(1, line2);
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
    const char *title;

    if (notification->kind == FARM_NOTIFICATION_ANIMAL) {
        title = "Nuevo Animal";
    } else {
        title = "Nuevo Producto";
    }

    Display_PutNumber(notification->number);
    Display_ShowText(title, displayLine);
}

static void Display_ShowIdleScreen (void) {
    unsigned char day;
    unsigned char digit;
    unsigned char month;

    if (Farm_IsConfigured() == 0) {
        Display_ShowText("Esperando Init", "");
        return;
    }

    if (SerialTime_IsConfigured() == 0) {
        Display_ShowText(Farm_GetName(), "Falta Hora");
        return;
    }

    day = SerialTime_GetDay();
    month = SerialTime_GetMonth();
    digit = 0;
    while (day >= 10) {
        day = (unsigned char)(day - 10);
        digit++;
    }
    displayLine[0] = (char)('0' + digit);
    displayLine[1] = (char)('0' + day);
    displayLine[2] = '/';
    digit = 0;
    while (month >= 10) {
        month = (unsigned char)(month - 10);
        digit++;
    }
    displayLine[3] = (char)('0' + digit);
    displayLine[4] = (char)('0' + month);
    displayLine[5] = '\0';
    Display_ShowText(Farm_GetName(), displayLine);
}

void Display_Init (void) {
    TI_NewTimer((unsigned char *)&displayCtrl);
    DISPLAY_SET_STATE(0);
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
