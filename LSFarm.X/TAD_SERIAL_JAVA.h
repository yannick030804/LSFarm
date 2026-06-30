#ifndef TAD_SERIAL_JAVA_H
#define TAD_SERIAL_JAVA_H

#define CONFIG_SERIAL_JAVA TRISCbits.TRISC6 = 0; TRISCbits.TRISC7 = 1
#define JAVA_TX LATCbits.LATC6
#define JAVA_RX PORTCbits.RC7

void SerialJava_Init (void);
unsigned char SJ_PutString( const char *str);
const char *SJ_GetLine (void);

#endif
