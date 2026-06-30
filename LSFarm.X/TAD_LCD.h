#ifndef TAD_LCD_H
#define TAD_LCD_H

#define CONFIG_LCD TRISDbits.TRISD1 = 0; TRISDbits.TRISD2 = 0; TRISDbits.TRISD3 = 0; TRISDbits.TRISD4 = 0; TRISDbits.TRISD5 = 0; TRISDbits.TRISD6 = 0

void LCD_Init (void);
void motorLCD (void);

#endif
