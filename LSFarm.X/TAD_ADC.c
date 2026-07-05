/*
 * TAD_ADC
 * LSFarm P2FB - Sistemes Digitals i Microprocessadors
 *
 * @author: Felipe Trejos & Daniel Leo Powell（◕‿◕）
 */

#include <xc.h>
#include "TAD_ADC.h"

void ADC_Init(void)
{
    ADCON2 = 0x35;   // left-justify, 16 TAD de adquisicion, Fosc/16
    ADCON0 = 0x01;   // ADC encendido
}

unsigned char ADC_Start(unsigned char channel)
{
    if (ADCON0bits.GO) return 0;
    
    ADCON0 = (unsigned char)((ADCON0 & 0xC3) | (unsigned char)(channel << 2));   // cambia canal sin tocar GO ni ADON
    ADCON0bits.GO = 1;
    return 1;
}
