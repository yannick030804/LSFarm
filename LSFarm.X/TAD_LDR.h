#ifndef TAD_LDR_H
#define TAD_LDR_H

#define CONFIG_LDR TRISAbits.TRISA3 = 1
#define LDR PORTAbits.RA3

void LDR_Init (void);
void motorLDR (void);

#endif
