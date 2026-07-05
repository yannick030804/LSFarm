#ifndef TAD_SERIAL_TIME_H
#define TAD_SERIAL_TIME_H

#define CONFIG_SERIAL_TIME TRISBbits.TRISB1 = 0; TRISBbits.TRISB2 = 1; LATBbits.LATB1 = 1

extern unsigned char serialTimeConfigured;
extern unsigned char serialTimeDay;
extern unsigned char serialTimeMonth;
extern unsigned char serialTimeHour;
extern unsigned char serialTimeMinute;
extern unsigned char serialTimeSecond;

#define SerialTime_IsConfigured() (serialTimeConfigured)
#define SerialTime_GetDay() (serialTimeDay)
#define SerialTime_GetMonth() (serialTimeMonth)
#define SerialTime_GetHour() (serialTimeHour)
#define SerialTime_GetMinute() (serialTimeMinute)
#define SerialTime_GetSecond() (serialTimeSecond)

void SerialTime_Init (void);
void motorSerialTime (void);
void SerialTime_StartBitISR (void);
void SerialTime_TickISR (void);

#endif
