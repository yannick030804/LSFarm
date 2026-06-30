#ifndef TAD_HEARTBEAT_H
#define TAD_HEARTBEAT_H

#define CONFIG_HEARTBEAT TRISAbits.TRISA4 = 0; LATAbits.LATA4 = 0
#define HEARTBEAT LATAbits.LATA4

void Heartbeat_Init (void);
void motorHeartbeat (void);

#endif
