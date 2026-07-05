#include <xc.h>
#include "TAD_LCD.h"
#include "TAD_TIMER.h"

#define RSUp()       (LATDbits.LATD6 = 1)
#define RSDown()     (LATDbits.LATD6 = 0)
#define EnableUp()   (LATDbits.LATD1 = 1)
#define EnableDown() (LATDbits.LATD1 = 0)

static char frame[32];
static unsigned char cursor;
static unsigned char scan;
static unsigned char timerHandle;

static void LCD_SendByte(char value, unsigned char isData)
{
    if (isData == 1) {
        RSUp();
    } else {
        RSDown();
    }
    EnableUp();
    LATD = (LATD & 0xC3) | ((value >> 2) & 0x3C);
    EnableDown();
    EnableUp();
    LATD = (LATD & 0xC3) | ((value << 2) & 0x3C);
    EnableDown();
}

static void EscriuPrimeraOrdre(char ordre)
{
    RSDown();
    EnableUp();
    LATDbits.LATD3 = (ordre & 0x02) ? 1 : 0;
    LATDbits.LATD2 = (ordre & 0x01) ? 1 : 0;
    EnableDown();
}

static void EsperaMs(unsigned char ms)
{
    TI_ResetTics(timerHandle);
    while (TI_GetTics(timerHandle) < ms) {
    }
}

void LCD_Init(void) {
    unsigned char i;

    CONFIG_LCD;
    TI_NewTimer(&timerHandle);
    RSDown();
    EnableDown();

    for (i = 0; i < 32; i++) {
        frame[i] = ' ';
    }
    cursor = 0;
    scan = 0;

    EsperaMs(100);
    EscriuPrimeraOrdre(0x03);
    EsperaMs(15);
    EscriuPrimeraOrdre(0x03);
    EsperaMs(5);
    EscriuPrimeraOrdre(0x03);
    EsperaMs(5);
    EscriuPrimeraOrdre(0x02);
    EsperaMs(1);
    LCD_SendByte(0x28, 0);
    EsperaMs(2);
    LCD_SendByte(0x08, 0);
    EsperaMs(2);
    LCD_SendByte(0x01, 0);
    EsperaMs(3);
    LCD_SendByte(0x06, 0);
    EsperaMs(2);
    LCD_SendByte(0x0C, 0);
}

void LCD_GotoXY(unsigned char column, unsigned char row)
{
    cursor = (unsigned char)(column + (row ? 16 : 0));
}

void LCD_PutChar(char c)
{
    frame[cursor] = c;
    cursor++;
}

void motorLCD (void) {
    static unsigned char state = 0;

    if (TI_GetTics(timerHandle) < 1) {
        return;
    }
    TI_ResetTics(timerHandle);

    switch (state) {
        case 0:
            LCD_SendByte(0x80, 0);
            scan = 0;
            state++;
            break;
        case 1:
            LCD_SendByte(frame[scan], 1);
            scan++;
            if (scan == 16) {
                state++;
            }
            break;
        case 2:
            LCD_SendByte(0xC0, 0);
            state++;
            break;
        case 3:
            LCD_SendByte(frame[scan], 1);
            scan++;
            if (scan == 32) {
                state = 0;
            }
            break;
    }
}
