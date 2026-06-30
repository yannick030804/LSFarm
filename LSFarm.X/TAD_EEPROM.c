#include <xc.h>
#include "TAD_EEPROM.h"

static unsigned char busy;

void EEPROM_Init (void) {
    busy = 0;
}

void motorEEPROM (void) {
    static unsigned char state = 0;

    switch (state) {
        
    }
}
