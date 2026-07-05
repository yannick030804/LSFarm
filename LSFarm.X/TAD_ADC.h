#ifndef TAD_ADC_H
#define TAD_ADC_H
 
 void ADC_Init(void);
 
 unsigned char ADC_Start(unsigned char channel);
 
 unsigned char ADC_IsDone(void);
 
 unsigned char ADC_Read(void);
 
 #endif
