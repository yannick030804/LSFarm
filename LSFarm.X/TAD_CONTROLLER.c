#include <xc.h>
#include "TAD_CONTROLLER.h"
#include "TAD_TIMER.h"

static unsigned char timerHandle; 

void Controller_Init (void) {
    TI_NewTimer(&timerHandle);
}


void motorController (void) {
    static unsigned char state = 0;

    switch (state) {
        
    }
}