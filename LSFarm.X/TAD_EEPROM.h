#ifndef TAD_EEPROM_H
#define TAD_EEPROM_H

extern unsigned char EEPROM_Mode;

#define EEPROM_Init()
#define EEPROM_IsBusy() (EEPROM_Mode != 0)

void motorEEPROM (void);
unsigned char EEPROM_ReadByte (unsigned char addr);
unsigned char EEPROM_StartByteWrite (unsigned char addr, unsigned char data);
void EEPROM_RequestClear (void);

#endif
