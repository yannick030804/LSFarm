#include <xc.h>
#include "TAD_CONTROLLER.h"
#include "TAD_TIMER.h"
#include "TAD_SERIAL_JAVA.h"

static unsigned char timerHandle;
static const char *line;

void Controller_Init (void) {
    TI_NewTimer(&timerHandle);
}


void motorController (void) {
    static unsigned char state = 0;

    switch (state) {
        case 0:
            line = SJ_GetLine();
            if (line != 0) {
                if (line[0] == 'I') {
                    state = 1;
                }
            }
            break;
        case 1:
            if (SJ_PutString("INIT OK\r\n")) {
                state = 0;
            }
            break;
    }
}