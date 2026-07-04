#include <xc.h>
#include "TAD_DISPLAY.h"
#include "TAD_FARM.h"
#include "TAD_LCD.h"
#include "TAD_SERIAL_TIME.h"
#include "TAD_TIMER.h"

#define DISPLAY_MESSAGE_TIME 3000UL
#define DISPLAY_TIMER_MASK 0x07
#define DISPLAY_STATE_SHIFT 3
#define DISPLAY_TIMER_HANDLE() ((unsigned char)(displayLine[16] & DISPLAY_TIMER_MASK))
#define DISPLAY_GET_STATE() ((unsigned char)((displayLine[16] >> DISPLAY_STATE_SHIFT) & 0x03))
#define DISPLAY_SET_STATE(value) (displayLine[16] = (char)((displayLine[16] & DISPLAY_TIMER_MASK) | ((value) << DISPLAY_STATE_SHIFT)))
#define DISPLAY_GET_LAST_DAY() ((unsigned char)displayLine[14])
#define DISPLAY_SET_LAST_DAY(value) (displayLine[14] = (char)(value))
#define DISPLAY_GET_LAST_MONTH() ((unsigned char)displayLine[15])
#define DISPLAY_SET_LAST_MONTH(value) (displayLine[15] = (char)(value))

static char displayLine[17];

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
    LCD_Clear();
    Display_PutFixedLine(0, line1);
    Display_PutFixedLine(1, line2);
}

static void Display_AppendNumber (char *line, unsigned char *index, unsigned char value) {
    unsigned char digit;
    unsigned char started = 0;

    digit = (unsigned char)(value / 100);
    if (digit != 0) {
        line[*index] = (char)('0' + digit);
        (*index)++;
        started = 1;
    }

    digit = (unsigned char)((value / 10) % 10);
    if (digit != 0 || started == 1) {
        line[*index] = (char)('0' + digit);
        (*index)++;
    }

    line[*index] = (char)('0' + (value % 10));
    (*index)++;
}

static void Display_ShowNotification (const FarmNotification *notification) {
    unsigned char index = 0;
    const char *name;

    if (notification->kind == FARM_NOTIFICATION_ANIMAL) {
        Display_ShowText("Nuevo Animal", "");
        if (notification->species == 0) {
            name = "Vaca ";
        } else if (notification->species == 1) {
            name = "Cerdo ";
        } else if (notification->species == 2) {
            name = "Caballo ";
        } else {
            name = "Gallina ";
        }
    } else {
        Display_ShowText("Nuevo Producto", "");
        if (notification->species == 0) {
            name = "Leche ";
        } else if (notification->species == 1) {
            name = "Jamon ";
        } else if (notification->species == 2) {
            name = "Pincel ";
        } else {
            name = "Huevo ";
        }
    }

    while (*name != '\0' && index < 16) {
        displayLine[index] = *name;
        index++;
        name++;
    }
    if (index < 16) {
        displayLine[index] = ':';
        index++;
    }
    if (index < 16) {
        displayLine[index] = ' ';
        index++;
    }
    if (index < 16) {
        Display_AppendNumber(displayLine, &index, notification->number);
    }
    while (index < 16) {
        displayLine[index] = ' ';
        index++;
    }
    Display_PutFixedLine(1, displayLine);
}

static void Display_ShowIdleScreen (void) {
    unsigned char farmConfigured = Farm_IsConfigured();
    unsigned char timeConfigured = SerialTime_IsConfigured();
    unsigned char day;
    unsigned char month;
    unsigned char index;

    if (farmConfigured == 0 && timeConfigured == 0) {
        Display_ShowText("Esperando", "Init y Hora");
        return;
    }

    if (farmConfigured == 0) {
        Display_ShowText("Esperando Init", "");
        return;
    }

    if (timeConfigured == 0) {
        Display_ShowText(Farm_GetName(), "Falta Hora");
        return;
    }

    day = SerialTime_GetDay();
    month = SerialTime_GetMonth();

    displayLine[0] = 'F';
    displayLine[1] = 'e';
    displayLine[2] = 'c';
    displayLine[3] = 'h';
    displayLine[4] = 'a';
    displayLine[5] = ' ';
    displayLine[6] = (char)('0' + (day / 10));
    displayLine[7] = (char)('0' + (day % 10));
    displayLine[8] = '/';
    displayLine[9] = (char)('0' + (month / 10));
    displayLine[10] = (char)('0' + (month % 10));
    for (index = 11; index < 16; index++) {
        displayLine[index] = ' ';
    }
    Display_ShowText(Farm_GetName(), displayLine);
}

void Display_Init (void) {
    TI_NewTimer((unsigned char *)&displayLine[16]);
    DISPLAY_SET_STATE(0);
}

void motorDisplay (void) {
    FarmNotification notification;
    unsigned char timeConfigured;
    unsigned char day;
    unsigned char month;

    switch (DISPLAY_GET_STATE()) {
        case 0:
            Display_ShowIdleScreen();
            timeConfigured = SerialTime_IsConfigured();
            if (timeConfigured == 1) {
                DISPLAY_SET_LAST_DAY(SerialTime_GetDay());
                DISPLAY_SET_LAST_MONTH(SerialTime_GetMonth());
            } else {
                DISPLAY_SET_LAST_DAY(255);
                DISPLAY_SET_LAST_MONTH(255);
            }
            DISPLAY_SET_STATE(1);
            break;
        case 1:
            if (Farm_GetNotification(&notification) == 1) {
                Display_ShowNotification(&notification);
                TI_ResetTics(DISPLAY_TIMER_HANDLE());
                DISPLAY_SET_STATE(2);
                break;
            }

            timeConfigured = SerialTime_IsConfigured();
            if (Farm_IsConfigured() == 0 || timeConfigured == 0) {
                Display_ShowIdleScreen();
                DISPLAY_SET_LAST_DAY(255);
                DISPLAY_SET_LAST_MONTH(255);
            } else {
                day = SerialTime_GetDay();
                month = SerialTime_GetMonth();
                if (day != DISPLAY_GET_LAST_DAY() || month != DISPLAY_GET_LAST_MONTH()) {
                    Display_ShowIdleScreen();
                    DISPLAY_SET_LAST_DAY(day);
                    DISPLAY_SET_LAST_MONTH(month);
                }
            }
            break;
        case 2:
            if (TI_GetTics(DISPLAY_TIMER_HANDLE()) >= DISPLAY_MESSAGE_TIME) {
                if (Farm_GetNotification(&notification) == 1) {
                    Display_ShowNotification(&notification);
                    TI_ResetTics(DISPLAY_TIMER_HANDLE());
                } else {
                    DISPLAY_SET_STATE(0);
                }
            }
            break;
    }
}
