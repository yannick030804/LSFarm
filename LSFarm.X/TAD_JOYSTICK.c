#include <xc.h>
#include "TAD_JOYSTICK.h"
#include "TAD_ADC.h"

#define JOY_LAST_MASK   0x0F
#define JOY_PENDING_MASK 0xF0
#define JOY_GET_LAST() ((unsigned char)(joyPacked & JOY_LAST_MASK))
#define JOY_SET_LAST(value) (joyPacked = (unsigned char)((joyPacked & JOY_PENDING_MASK) | ((value) & JOY_LAST_MASK)))
#define JOY_GET_PENDING() ((unsigned char)((joyPacked >> 4) & 0x0F))
#define JOY_SET_PENDING(value) (joyPacked = (unsigned char)((joyPacked & JOY_LAST_MASK) | (((value) & 0x0F) << 4)))

static unsigned char adcX;
static unsigned char adcY;
static unsigned char joyPacked;

static void processJoystick (void) {
    unsigned char newDirection = JOY_EVT_NONE;

    if (JOY_GET_PENDING() != JOY_EVT_NONE) {
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

    if (newDirection != JOY_GET_LAST()) {
        JOY_SET_LAST(newDirection);
        if (newDirection != JOY_EVT_NONE) {
            JOY_SET_PENDING(newDirection);
        }
    }
}

void Joystick_Init(void) {
    CONFIG_JOYSTICK;
    adcX = 128;
    adcY = 128;
    joyPacked = 0;
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
    unsigned char event = JOY_GET_PENDING();

    JOY_SET_PENDING(JOY_EVT_NONE);
    return event;
}
