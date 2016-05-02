/*
 * File:   main.c
 * Author: Daniel Barber-Cironi
 *         Jake Kruse
 *         Tim Markley
 *         Jared Ford
 *
 */

/*
 * UART3 - Radio
 * 
 */    
 
#include <string.h>
#include <stdio.h>
#include <xc.h>
#include "mcc_generated_files/mcc.h"
#include "timers.h"
#include "power.h"
#include "relay.h"
#include "inject.h"
#include "./utests/unit.h"

uint16_t AppTimer, GPSpowerTimer;

int main(void)
{
    //System Init
    SYSTEM_Initialize();

    //Components Init
    RelayInit(); //initialize the state of the relay machine to 0
    InjectInit();

    //Init timer
    AppTimer = 0; 

    while(1)
    {
        RelayLoop();
    }
}
