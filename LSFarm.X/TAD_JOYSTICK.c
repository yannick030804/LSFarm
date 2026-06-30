#include <xc.h>
#include "TAD_JOYSTICK.h"

static unsigned char adcX;
static unsigned char adcY;
static unsigned char pendingEvent;
static unsigned char lastDirection;

void Joystick_Init(void) {
    CONFIG_JOYSTICK;
    adcX = 128;
    adcY = 128;
    pendingEvent = 0;
    lastDirection = 0;
}

void motorJoystick (void) {
    static unsigned char state = 0;

    switch (state) {
        
    }
}
