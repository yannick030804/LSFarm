#ifndef TAD_JOYSTICK_H
#define TAD_JOYSTICK_H

#define CONFIG_JOYSTICK TRISAbits.TRISA0 = 1; TRISAbits.TRISA1 = 1
#define JOYSTICK_X PORTAbits.RA0
#define JOYSTICK_Y PORTAbits.RA1
#define JOYSTICK_X_CHANNEL 0
#define JOYSTICK_Y_CHANNEL 1

#define JOY_EVT_NONE  0
#define JOY_EVT_UP    1
#define JOY_EVT_DOWN  2
#define JOY_EVT_LEFT  3
#define JOY_EVT_RIGHT 4

void Joystick_Init (void);
void motorJoystick (void);
unsigned char Joystick_GetEvent (void);

#endif
