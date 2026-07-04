#include <xc.h>
#include "TAD_BUTTON.h"
#include "TAD_TIMER.h"

static unsigned char timerHandle;
static unsigned char buttonState;

void Button_Init (void) {
    CONFIG_BTN;
    TI_NewTimer(&timerHandle);
    buttonState = 0;
}

unsigned char getButton (void) {
    if (buttonState == 2) {
        buttonState = 3;
        return 1;
    }
    return 0;
}

void motorButton (void) {
    switch (buttonState) {
        case 0:
            if (BUTTON == 1) {
            } else if (BUTTON == 0) {
                buttonState = 1;
                TI_ResetTics(timerHandle);
            }
            break;
        case 1:
            if (BUTTON == 0 && TI_GetTics(timerHandle) >= REBOUNDS) {
                buttonState = 2;
                TI_ResetTics(timerHandle);
            } else if (BUTTON == 1) {
                buttonState = 0;
            }
            break;
        case 2:
            if (BUTTON == 1 && TI_GetTics(timerHandle) >= 8) {
                buttonState = 0;
            }
            break;
        case 3:
            if (BUTTON == 1 && TI_GetTics(timerHandle) >= 8) {
                buttonState = 0;
            }
            break;
    }
}
