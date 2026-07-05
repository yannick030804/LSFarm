#ifndef TAD_BUTTON_H
#define TAD_BUTTON_H

#define CONFIG_BTN TRISBbits.TRISB0 = 1
#define JOYSTICK_SW PORTBbits.RB0
#define BUTTON JOYSTICK_SW
#define REBOUNDS 16

void Button_Init (void);
unsigned char getButton (void);
void motorButton (void);
 
 #endif
