#ifndef TAD_EEPROM_H
#define TAD_EEPROM_H

void EEPROM_Init (void);
void motorEEPROM (void);
unsigned char EEPROM_ReadByte (unsigned char addr);
void EEPROM_ReadBlock (unsigned char addr, unsigned char *dst, unsigned char len);
unsigned char EEPROM_StartByteWrite (unsigned char addr, unsigned char data);
unsigned char EEPROM_StartImageWrite (unsigned char addr, const unsigned char *src, unsigned char len);
unsigned char EEPROM_IsBusy (void);
void EEPROM_RequestClear (void);

#endif
