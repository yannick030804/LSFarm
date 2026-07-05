#include <xc.h>
#include "TAD_LDR.h"
#include "TAD_ADC.h"
#include "TAD_FARM.h"
#include "TAD_TIMER.h"

#define LDR_ADC_CHANNEL 3
#define LDR_COVER_THRESHOLD 100
#define LDR_TIMEOUT_MS 5000UL

static unsigned char timerHandle;

void LDR_Init (void) {
    CONFIG_LDR;
    TI_NewTimer(&timerHandle);
}

void motorLDR (void) {
    static unsigned char state = 0;

    switch (state) {
        case 0:
            if (Farm_IsRestRequestPending() == 1) {
                TI_ResetTics(timerHandle);
                state++;
            }
            break;
        case 1:
            if (Farm_IsRestRequestPending() == 0) {
                state = 0;
                break;
            }
            if (ADC_Start(LDR_ADC_CHANNEL) == 1) {
                state++;
            }
            break;
        case 2:
            if (Farm_IsRestRequestPending() == 0) {
                state = 0;
                break;
            }
            if (ADC_IsDone() == 1) {
                if (ADC_Read() < LDR_COVER_THRESHOLD) {
                    Farm_NotifyRestSuccess();
                    state = 0;
                } else if (TI_GetTics(timerHandle) >= LDR_TIMEOUT_MS) {
                    Farm_NotifyRestTimeout();
                    state = 0;
                } else {
                    state = 1;
                }
            }
            break;
    }
}
