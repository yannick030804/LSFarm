/*
 * File:   main.c
 * Author: yanni
 *
 * Created on 24 de febrero de 2026, 23:35
 */

#include <xc.h>
#include "TAD_ADC.h"
#include "TAD_BUTTON.h"
#include "TAD_CONTROLLER.h"
#include "TAD_DISPLAY.h"
#include "TAD_EEPROM.h"
#include "TAD_FARM.h"
#include "TAD_HEARTBEAT.h"
#include "TAD_JOYSTICK.h"
#include "TAD_LCD.h"
#include "TAD_LDR.h"
#include "TAD_PROTOCOL.h"
#include "TAD_SERIAL_JAVA.h"
#include "TAD_SERIAL_TIME.h"
#include "TAD_TIMER.h"

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

void PORT_Init (void) {
    ADCON1 = 0x0B;
}

void main (void) {
    
    TI_Init();
    PORT_Init();
    ADC_Init();
    Button_Init();
    Controller_Init();
    Display_Init();
    EEPROM_Init();
    Farm_Init();
    Heartbeat_Init();
    Joystick_Init();
    LCD_Init();
    LDR_Init();
    Protocol_Init();
    SerialJava_Init();
    SerialTime_Init();
    
    while (1) {
        motorButton();
        motorController();
        motorDisplay();
        motorEEPROM();
        motorFarm();
        motorHeartbeat();
        motorJoystick();
        motorLDR();
        motorSerialTime();
    }
}
