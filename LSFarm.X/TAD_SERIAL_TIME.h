#ifndef TAD_SERIAL_TIME_H
#define TAD_SERIAL_TIME_H

#define CONFIG_SERIAL_TIME TRISBbits.TRISB1 = 0; TRISBbits.TRISB2 = 1; LATBbits.LATB1 = 1

void SerialTime_Init (void);
void motorSerialTime (void);
void SerialTime_StartBitISR (void);
void SerialTime_TickISR (void);
unsigned char SerialTime_IsConfigured (void);
unsigned char SerialTime_GetDay (void);
unsigned char SerialTime_GetMonth (void);
unsigned char SerialTime_GetHour (void);
unsigned char SerialTime_GetMinute (void);
unsigned char SerialTime_GetSecond (void);

#endif
