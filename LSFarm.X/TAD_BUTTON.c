#include <xc.h>
#include "TAD_BUTTON.h"
#include "TAD_TIMER.h"

static unsigned char timerHandle;
static unsigned char isPressed;

void Button_Init (void) {
    CONFIG_BTN;
    TI_NewTimer(&timerHandle);
    isPressed = 0;
}

unsigned char getButton (void) {
    unsigned char aux = isPressed;
    isPressed = 0;
    return aux;
}

void motorButton (void) {
    static unsigned char state = 0;

    switch (state) {
        case 0:
            if (BUTTON == 0) {
                state++;
                TI_ResetTics(timerHandle);
            }
            break;
        case 1:
            if (BUTTON == 0 && TI_GetTics(timerHandle) >= REBOUNDS) {
                isPressed = 1;
                state++;
                TI_ResetTics(timerHandle);
            }
            break;
        case 2:
            if (BUTTON == 1 && TI_GetTics(timerHandle) >= 8) {
                state = 0;
            }
            break;
    }
}
