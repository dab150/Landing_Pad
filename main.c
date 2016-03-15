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
    run_all_tests();


    //Components Init
    RelayInit();
    InjectInit();
    gps_Init();
    Bar_Init();
    
    
    //gps_PowerOn(2500);




    //Init timer
    AppTimer = 0;

    

    while(1)
    {
        RelayLoop();
        InjectTryInject();
    }
}



/**

 SYSTEM_Initialize();
    RelayInit();
    InjectInit();

    //Bar_Init();

    //POWER_GPS_COMPASS_TURN_ON();
    //Compass_Init();
    //gps_Init();


    while(1)
    {
        //relayUARTs();
        
        __Bar_Test();
        __Compass_Test();

        //Delays 3 sec
        for(i=0; i<(10 * 3); i++)
            delay_ms(100);
         * */

