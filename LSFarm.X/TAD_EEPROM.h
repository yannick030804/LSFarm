#ifndef TAD_EEPROM_H
#define TAD_EEPROM_H

extern unsigned char eepromMode;

#define EEPROM_Init()
void motorEEPROM (void);
unsigned char EEPROM_ReadByte (unsigned char addr);
unsigned char EEPROM_StartByteWrite (unsigned char addr, unsigned char data);
#define EEPROM_IsBusy() ((unsigned char)(eepromMode != 0))
void EEPROM_RequestClear (void);

#endif
