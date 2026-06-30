#ifndef TAD_JOYSTICK_H
#define TAD_JOYSTICK_H

#define CONFIG_JOYSTICK TRISAbits.TRISA0 = 1; TRISAbits.TRISA1 = 1
#define JOYSTICK_X PORTAbits.RA0
#define JOYSTICK_Y PORTAbits.RA1

void Joystick_Init (void);
void motorJoystick (void);

#endif
