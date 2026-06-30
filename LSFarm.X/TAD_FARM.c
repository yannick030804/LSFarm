#include <xc.h>
#include "TAD_FARM.h"
#include "TAD_TIMER.h"

static unsigned char timerHandle;
static unsigned char configured;
static unsigned char timeReady;

void Farm_Init (void) {
    TI_NewTimer(&timerHandle);
    configured = 0;
    timeReady = 0;
}

void motorFarm (void) {
    static unsigned char state = 0;

    switch (state) {
        
    }
}