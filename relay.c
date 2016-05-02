/******************************************************************************/
/* Files to Include                                                           */
/******************************************************************************/

#if defined(__XC)
    #include <xc.h>         /* XC8 General Include File */
#elif defined(HI_TECH_C)
    #include <htc.h>        /* HiTech General Include File */
#endif

#include <stdint.h>        /* For uint8_t definition */
#include <stdbool.h>       /* For true/false definition */

#include "relay.h"
#include "mcc_generated_files/pin_manager.h"
#include "mcc_generated_files/uart1.h"
#include "mcc_generated_files/uart3.h"
#include "inject.h"

/******************************************************************************/
/* Variable                                                                   */
/******************************************************************************/

relay_data_t relayData;

/******************************************************************************/
/* Functions                                                                  */
/******************************************************************************/

/**
 * Init State machine
 */
void RelayInit()
{
    //Init state
    relayData.state = 0;
}

/**
 * Main code for the Relay Machine
 */
void RelayLoop()
{
    if(relayData.state > RELAY_STATE_ERROR)
    {
        RelayInit();
        return;
    }
    
    switch (relayData.state)
    {
        case RELAY_STATE_INIT:
                relayData.state = RELAY_STATE_RUNNING;
            break;

        case RELAY_STATE_RUNNING:
            relayFromRadio();
            
        case RELAY_STATE_ERROR:
        default:
            //Error, unknown state
            RelayInit();
            break; 
    }
}

/**
 * Relays data from Radio to USB
 */
void relayFromRadio()
{

    /* UART3 - Radio        */

    if(!UART3_ReceiveBufferIsEmpty())       //New data on UART3 (Radio)
    {
        uint8_t rxByte = UART3_Read();      //Read byte from remote radio
        int desiredPacket = 0;
        desiredPacket = CheckPacketLoop(rxByte); //check  if this data corresponds to the packet that we want
        
        if(desiredPacket)                   //packet that we are looking for
        {
            InjectTryInject();              //create and inject our packet
        }
        
    }
}

