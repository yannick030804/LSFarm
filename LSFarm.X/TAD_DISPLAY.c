#include <xc.h>
#include "TAD_DISPLAY.h"
#include "TAD_FARM.h"
#include "TAD_LCD.h"
#include "TAD_SERIAL_TIME.h"
#include "TAD_TIMER.h"

#define DISPLAY_MESSAGE_TIME 3000UL

static unsigned char timerHandle;
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
    displayLine[16] = '\0';
    Display_PutFixedLine(1, displayLine);
}

static void Display_ShowIdleScreen (void) {
    unsigned char index;

    if (Farm_IsConfigured() == 0 && SerialTime_IsConfigured() == 0) {
        Display_ShowText("Esperando", "Init y Hora");
        return;
    }

    if (Farm_IsConfigured() == 0) {
        Display_ShowText("Esperando Init", "");
        return;
    }

    if (SerialTime_IsConfigured() == 0) {
        Display_ShowText(Farm_GetName(), "Falta Hora");
        return;
    }

    displayLine[0] = 'F';
    displayLine[1] = 'e';
    displayLine[2] = 'c';
    displayLine[3] = 'h';
    displayLine[4] = 'a';
    displayLine[5] = ' ';
    displayLine[6] = (char)('0' + (SerialTime_GetDay() / 10));
    displayLine[7] = (char)('0' + (SerialTime_GetDay() % 10));
    displayLine[8] = '/';
    displayLine[9] = (char)('0' + (SerialTime_GetMonth() / 10));
    displayLine[10] = (char)('0' + (SerialTime_GetMonth() % 10));
    for (index = 11; index < 16; index++) {
        displayLine[index] = ' ';
    }
    displayLine[16] = '\0';

    Display_ShowText(Farm_GetName(), displayLine);
}

void Display_Init (void) {
    TI_NewTimer(&timerHandle);
}

void motorDisplay (void) {
    static unsigned char state = 0;
    static unsigned char lastDay = 255;
    static unsigned char lastMonth = 255;
    FarmNotification notification;

    switch (state) {
        case 0:
            Display_ShowIdleScreen();
            if (SerialTime_IsConfigured() == 1) {
                lastDay = SerialTime_GetDay();
                lastMonth = SerialTime_GetMonth();
            } else {
                lastDay = 255;
                lastMonth = 255;
            }
            state = 1;
            break;
        case 1:
            if (Farm_GetNotification(&notification) == 1) {
                Display_ShowNotification(&notification);
                TI_ResetTics(timerHandle);
                state = 2;
                break;
            }

            if (Farm_IsConfigured() == 0 || SerialTime_IsConfigured() == 0) {
                Display_ShowIdleScreen();
                lastDay = 255;
                lastMonth = 255;
            } else if (SerialTime_GetDay() != lastDay || SerialTime_GetMonth() != lastMonth) {
                Display_ShowIdleScreen();
                lastDay = SerialTime_GetDay();
                lastMonth = SerialTime_GetMonth();
            }
            break;
        case 2:
            if (TI_GetTics(timerHandle) >= DISPLAY_MESSAGE_TIME) {
                if (Farm_GetNotification(&notification) == 1) {
                    Display_ShowNotification(&notification);
                    TI_ResetTics(timerHandle);
                } else {
                    state = 0;
                }
            }
            break;
    }
}
