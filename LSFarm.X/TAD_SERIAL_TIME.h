#ifndef TAD_SERIAL_TIME_H
#define TAD_SERIAL_TIME_H

typedef struct {
    unsigned char day;
    unsigned char month;
    unsigned char hour;
    unsigned char minute;
    unsigned char second;
} STDate;

#define CONFIG_SERIAL_TIME TRISBbits.TRISB1 = 0; TRISBbits.TRISB2 = 1; LATBbits.LATB1 = 1
#define TIME_TX LATBbits.LATB1
#define TIME_RX PORTBbits.RB2

void SerialTime_Init (void);
void motorSerialTime (void);

#endif
