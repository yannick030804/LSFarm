#ifndef TAD_EEPROM_H
#define TAD_EEPROM_H

#define EEPROM_Init()
void motorEEPROM (void);
unsigned char EEPROM_ReadByte (unsigned char addr);
unsigned char EEPROM_StartByteWrite (unsigned char addr, unsigned char data);
unsigned char EEPROM_IsBusy (void);
void EEPROM_RequestClear (void);

#endif
