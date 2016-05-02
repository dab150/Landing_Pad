#ifndef INJECT_H
#define	INJECT_H

#include "CircularBuffer.h"

#define INJECT_PERIOD                   (60 * 2) //seconds

#define RADIO_OUTPUT_BUFFER             256

#define WP_CLEAR_MSG_LEN                4
#define WP_CLEAR_FULL_LEN               (WP_CLEAR_MSG_LEN + 6 + 2)

#define WP_CLEAR_SYSTEM_ID              0x01
#define WP_CLEAR_COMP_ID                191

/******************************************************************************/
/* TypeDefs                                                                   */
/******************************************************************************/

typedef enum
{
    INJECT_STATE_MSG_END = 0,    	// <-- Init state is 0
    INJECT_STATE_WAIT_LEN,
    INJECT_STATE_WAIT_END,
    MACHINE_NAME_STATE_MAX
}inject_states_t;


//the following union is based off of the MAVLink Packet Structure
typedef union
{
    uint8_t buf[WP_CLEAR_FULL_LEN];
    struct
    {
        uint8_t     start;            
        uint8_t     len;                
        uint8_t     count;             
        uint8_t     sys_id;            
        uint8_t     comp_id;           
        uint8_t     msgType;            
                                        
        uint8_t     data[WP_CLEAR_MSG_LEN];
        uint8_t     checksum[2]; 
    };
}packet_t;

typedef struct
{    
    uint8_t timer;   
    CircularBuffer_t outBuff;               //Buffer to store TX data software
                                            //will send over Radio UART
}inject_t;


typedef struct
{
    inject_states_t state;
    uint8_t bufArray[RADIO_OUTPUT_BUFFER];  //Array for a circular buffer
    uint16_t fullLen;       //How long is the actual packet
    uint16_t cont;          //How many bytes of the actual buffer I already received
    packet_t packet;        //packet to send
    
}inject_private_t;

/******************************************************************************/
/* Variable                                                                   */
/******************************************************************************/

extern inject_t inject;


/******************************************************************************/
/* Functions                                                                  */
/******************************************************************************/

void InjectInit();
void InjectLoop(uint8_t rx);
void InjectTryInject();


#endif	

