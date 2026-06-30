#include <xc.h>
#include "TAD_PROTOCOL.h"
#include "TAD_TIMER.h"

static unsigned char timerHandle;

void Protocol_Init(void) {
    TI_NewTimer(&timerHandle);
}
