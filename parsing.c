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
#include "stdio.h"
#include "string.h"
#include "inject.h"
#include "mcc_generated_files/uart1.h"
#include "barometer.h"
#include "parsing.h"
#include "relay.h"


#define X25_INIT_CRC        0xffff
#define X25_VALIDATE_CRC    0xf0b8

/******************************************************************************/
/* Local Variables                                                            */
/******************************************************************************/
/**
 * Local variable. Stores all the local data inject module uses
 */

static inject_private_t CheckPacket;

bool PACKET_DETECTED = false;

/******************************************************************************/
/* Local Functions                                                            */
/******************************************************************************/

void CheckPacketLoopInit()
{
    //Init state
    CheckPacket.state = 0;
}

/******************************************************************************/
/* Public Functions                                                           */
/******************************************************************************/

int CheckPacketLoop(unsigned char rx)
{
    if(CheckPacket.state > MACHINE_NAME_STATE_MAX)
    {
        CheckPacketLoopInit();
        return 0;
    }

    //Analyze received byte
    switch (CheckPacket.state)
    {
        case INJECT_STATE_MSG_END:
            if(rx == 0xFE)
            {
                //First byte of the packet detected
                CheckPacket.state = INJECT_STATE_WAIT_LEN;
            }
            break;

        case INJECT_STATE_WAIT_LEN:
            //Len byte received
            CheckPacket.cont = 2;                    //Already has 2 bytes
            CheckPacket.fullLen = rx + 6 + 2;        //Calculate the packet length
            CheckPacket.state = INJECT_STATE_WAIT_END;
            break;

        case INJECT_STATE_WAIT_END:
            
            if(++CheckPacket.cont >= CheckPacket.fullLen)
            {
                //Full packet received
                CheckPacket.state = INJECT_STATE_MSG_END;
            }

            if(CheckPacket.cont == 6 && rx == 242) //if the MessageID is 0xF2 (242) then it is a home position packet
            {                                      //this should be sufficient to determine if this is the packet we are looking for
                PACKET_DETECTED = true;
                return 1;
            }
            break;

        case MACHINE_NAME_STATE_MAX:
        default:
            //Error, unknown state
            CheckPacketLoopInit();
            break;
    }
    return 0; //default
}