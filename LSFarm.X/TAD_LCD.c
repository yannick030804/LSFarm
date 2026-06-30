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

static void CantaPartAlta(char c)
{
    LATD = (LATD & 0xC3) | ((c >> 2) & 0x3C);
}

static void CantaPartBaixa(char c)
{
    LATD = (LATD & 0xC3) | ((c << 2) & 0x3C);
}

static void CantaIR(char ir)
{
    RSDown();
    EnableUp();
    CantaPartAlta(ir);
    EnableDown();
    EnableUp();
    CantaPartBaixa(ir);
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
    CantaIR(0x28);
    EsperaMs(2);
    CantaIR(0x08);
    EsperaMs(2);
    CantaIR(0x01);
    EsperaMs(3);
    CantaIR(0x06);
    EsperaMs(2);
    CantaIR(0x0C);
}

void motorLCD (void) {
    static unsigned char state = 0;

    switch (state) {
        
    }
}
