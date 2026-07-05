#include <xc.h>
#include "TAD_JOYSTICK.h"
#include "TAD_ADC.h"

static unsigned char adcX;
static unsigned char adcY;
static unsigned char joyLast;
static unsigned char joyPending;

void Joystick_Init(void) {
    CONFIG_JOYSTICK;
}

void motorJoystick (void) {
    static unsigned char state = 0;

    switch (state) {
        case 0:
            if (ADC_Start(JOYSTICK_X_CHANNEL) == 1) {
                state++;
            }
            break;
        case 1:
            if (ADC_IsDone() == 1) {
                adcX = ADC_Read();
                state++;
            }
            break;
        case 2:
            if (ADC_Start(JOYSTICK_Y_CHANNEL) == 1) {
                state++;
            }
            break;
        case 3:
            if (ADC_IsDone() == 1) {
                adcY = ADC_Read();
                state++;
            }
            break;
        case 4:
            if (joyPending == JOY_EVT_NONE) {
                unsigned char newDirection = JOY_EVT_NONE;

                if (adcX > 170) {
                    newDirection = JOY_EVT_RIGHT;
                } else if (adcX < 86) {
                    newDirection = JOY_EVT_LEFT;
                } else if (adcY > 170) {
                    newDirection = JOY_EVT_DOWN;
                } else if (adcY < 86) {
                    newDirection = JOY_EVT_UP;
                }

                if (newDirection != joyLast) {
                    joyLast = newDirection;
                    if (newDirection != JOY_EVT_NONE) {
                        joyPending = newDirection;
                    }
                }
            }
            state = 0;
            break;
    }
}

unsigned char Joystick_GetEvent (void) {
    unsigned char event = joyPending;

    joyPending = JOY_EVT_NONE;
    return event;
}
