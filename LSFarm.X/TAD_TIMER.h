#ifndef TAD_TIMER_H
#define	TAD_TIMER_H

#define TI_FALS 0
#define TI_CERT 1

void RSI_Timer0 (void);

void TI_Init (void);

unsigned char TI_NewTimer (unsigned char *TimerHandle);

void TI_ResetTics (unsigned char TimerHandle);

unsigned long TI_GetTics (unsigned char TimerHandle);

void TI_CloseTimer (unsigned char TimerHandle);

#endif