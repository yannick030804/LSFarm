#include <xc.h>
#include "TAD_JOYSTICK.h"
#include "TAD_ADC.h"

static unsigned char adcX;
static unsigned char adcY;
static unsigned char pendingEvent;
static unsigned char lastDirection;

static void processJoystick (void) {
    unsigned char newDirection = JOY_EVT_NONE;

    if (pendingEvent != JOY_EVT_NONE) {
        return;
    }

    if (adcX > 170) {
        newDirection = JOY_EVT_RIGHT;
    } else if (adcX < 86) {
        newDirection = JOY_EVT_LEFT;
    } else if (adcY > 170) {
        newDirection = JOY_EVT_DOWN;
    } else if (adcY < 86) {
        newDirection = JOY_EVT_UP;
    }

    if (newDirection != lastDirection) {
        lastDirection = newDirection;
        if (newDirection != JOY_EVT_NONE) {
            pendingEvent = newDirection;
        }
    }
}

void Joystick_Init(void) {
    CONFIG_JOYSTICK;
    adcX = 128;
    adcY = 128;
    pendingEvent = JOY_EVT_NONE;
    lastDirection = JOY_EVT_NONE;
}

void motorJoystick (void) {
    static unsigned char state = 0;

    switch (state) {
        case 0:
            if (ADC_Start(JOYSTICK_X_CHANNEL) == 1) {
                state = 1;
            }
            break;
        case 1:
            if (ADC_IsDone() == 1) {
                adcX = ADC_Read();
                state = 2;
            }
            break;
        case 2:
            if (ADC_Start(JOYSTICK_Y_CHANNEL) == 1) {
                state = 3;
            }
            break;
        case 3:
            if (ADC_IsDone() == 1) {
                adcY = ADC_Read();
                state = 4;
            }
            break;
        case 4:
            processJoystick();
            state = 0;
            break;
    }
}

unsigned char Joystick_GetEvent (void) {
    unsigned char event = pendingEvent;

    pendingEvent = JOY_EVT_NONE;
    return event;
}
