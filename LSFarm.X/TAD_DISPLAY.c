#include <xc.h>
#include "TAD_DISPLAY.h"
#include "TAD_TIMER.h"

static unsigned char timerHandle;

void Display_Init (void) {
    TI_NewTimer(&timerHandle);
}

void motorDisplay (void) {
    static unsigned char state = 0;

    switch (state) {
        
    }
}