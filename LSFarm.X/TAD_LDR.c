#include <xc.h>
#include "TAD_LDR.h"

static unsigned char lightLevel;

void LDR_Init(void) {
    CONFIG_LDR;
    lightLevel = 255;
}

void motorLDR (void) {
    static unsigned char state = 0;

    switch (state) {
        
    }
}
