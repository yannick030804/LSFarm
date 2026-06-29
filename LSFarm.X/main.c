/*
 * File:   main.c
 * Author: yanni
 *
 * Created on 24 de febrero de 2026, 23:35
 */

#include <xc.h>

#pragma config OSC = HS
#pragma config PBADEN = DIG
#pragma config WDT = OFF
#pragma config MCLRE = ON
#pragma config DEBUG = OFF
#pragma config PWRT = OFF
#pragma config BOR = OFF
#pragma config LVP = OFF

void __interrupt () RSI_HIGH (void) {
    if(INTCONbits.TMR0IF == 1) {
        RSI_Timer0();
    }
}

void main (void) {
    
    TI_Init();
    
    while (1) {
      
    }
}
