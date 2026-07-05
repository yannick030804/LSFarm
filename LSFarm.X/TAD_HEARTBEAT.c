#include <xc.h>
#include "TAD_HEARTBEAT.h"
#include "TAD_TIMER.h"

static unsigned char timerHandle;
unsigned char heartbeatRebellion;

void Heartbeat_Init (void) {
    CONFIG_HEARTBEAT;
    TI_NewTimer(&timerHandle);
    HEARTBEAT = 0;
    TI_ResetTics(timerHandle);
}

void motorHeartbeat (void) {
    static unsigned char state = 0;

    if (heartbeatRebellion == 1) {
        HEARTBEAT = 0;
        TI_ResetTics(timerHandle);
        state = 0;
        return;
    }

    switch (state) {
        case 0:
            if (TI_GetTics(timerHandle) >= 700) {
                TI_ResetTics(timerHandle);
                HEARTBEAT = 1;
                state++;
            }
            break;
        case 1:
            if (TI_GetTics(timerHandle) >= 70) {
                TI_ResetTics(timerHandle);
                HEARTBEAT = 0;
                state++;
            }
            break;
        case 2:
            if (TI_GetTics(timerHandle) >= 120) {
                TI_ResetTics(timerHandle);
                HEARTBEAT = 1;
                state++;
            }
            break;
        case 3:
            if (TI_GetTics(timerHandle) >= 70) {
                TI_ResetTics(timerHandle);
                HEARTBEAT = 0;
                state = 0;
            }
            break;
    }
}
