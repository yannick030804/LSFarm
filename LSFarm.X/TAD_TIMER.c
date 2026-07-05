/*
 * File:   TAD_Timer.c
 * Author: yanni
 *
 * Created on 24 de febrero de 2026, 23:38
 */


#include <xc.h>
#include "TAD_TIMER.h"

#define T0CON_CONFIG 0x88
#define RECARREGA_TMR0 63036 // 1 ms con Fosc = 10 MHz

#define TI_NUMTIMERS 7

static unsigned int TimerStarts[TI_NUMTIMERS];
static unsigned char TimerBusyMask;

static volatile unsigned int Tics = 0;

void RSI_Timer0 () {
    TMR0 = RECARREGA_TMR0;
    TMR0IF = 0;
    Tics++;
}

void TI_Init () {
    TimerBusyMask = 0;
    T0CON = T0CON_CONFIG;
    TMR0 = RECARREGA_TMR0;
    INTCONbits.TMR0IF = 0;
    INTCONbits.TMR0IE = 1;
    INTCONbits.PEIE = 1;
    INTCONbits.GIE = 1;
}

unsigned char TI_NewTimer (unsigned char *TimerHandle) {
    unsigned char Comptador = 0;
    while((TimerBusyMask & (1u << Comptador)) != 0) {
        if(++Comptador == TI_NUMTIMERS) return (TI_FALS);
    }
    TimerBusyMask |= (unsigned char)(1u << Comptador);
    *TimerHandle = Comptador;
    return (TI_CERT);
}

void TI_ResetTics (unsigned char TimerHandle) {
    di();
    TimerStarts[TimerHandle] = Tics;
    ei();
}

unsigned int TI_GetTics (unsigned char TimerHandle) {
    di();
    unsigned int CopiaTicsActual = Tics;
    ei();
    return (unsigned int)(CopiaTicsActual - TimerStarts[TimerHandle]);
}
