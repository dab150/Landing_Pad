/*
 * File:   main.c
 * Author: ruslanOffice
 *
 * Created on January 13, 2015, 3:53 PM
 */

/*
 * UART1 - FTDI USB
 * UART2 - Bluetooth
 * UART3 - Radio
 * 
 */    
 
#include <string.h>
#include <stdio.h>
#include <xc.h>
#include "mcc_generated_files/mcc.h"
#include "barometer.h"
#include "timers.h"
#include "debug.h"
#include "compass.h"
#include "power.h"
#include "gps.h"
#include "relay.h"
#include "inject.h"
#include "./utests/unit.h"

uint16_t AppTimer, GPSpowerTimer;

int main(void)
{
    //System Init
    SYSTEM_Initialize();

    //Components Init
    RelayInit();
    InjectInit();

    //Init timer
    AppTimer = 0; 

    while(1)
    {
        RelayLoop();
        InjectTryInject();
    }
}
