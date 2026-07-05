#ifndef TAD_ADC_H
#define TAD_ADC_H

#include <xc.h>
 
 void ADC_Init(void);
 
 unsigned char ADC_Start(unsigned char channel);
 
 #define ADC_IsDone() (!ADCON0bits.GO)
 
 #define ADC_Read() (ADRESH)
 
 #endif
