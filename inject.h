/******************************************************************************
 *
 *                             PIC16 LIBRARY
 *
 * File: StateMachine.c/h
 * Version: 1.02
 * Author: ruslan
 *
 * Change list:
 * ------------------------------------------------------
 *  - v1.00 - Original version
 *  - v1.01 - Variables are inside a struct
              States are defined as enum
 *  - v1.02 - More states added
 ******************************************************************************/

#ifndef INJECT_H
#define	INJECT_H

#include "CircularBuffer.h"

#define INJECT_PERIOD                   (60 * 2) //seconds

#define RADIO_OUTPUT_BUFFER             256
#define PARAM_UPDATE_MSG_LEN            23
#define PARAM_UPDATE_FULL_LEN           (PARAM_UPDATE_MSG_LEN + 6 + 2)

#define PRESS_SYSTEM_ID                 0xE3
#define PRESS_COMP_ID                   191

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


typedef union
{
    uint8_t buf[PARAM_UPDATE_FULL_LEN];
    struct
    {
        uint8_t     start;              //0 - 1
        uint8_t     len;                //1 - 2
        uint8_t     count;              //2 - 3
        uint8_t     sys_id;             //3 - 4
        uint8_t     comp_id;            //4 - 5
        uint8_t     msgType;            //5 - 6
                                        //6 - 29
        uint8_t     data[PARAM_UPDATE_MSG_LEN];
        uint8_t     checksum[2];        //29- 31
    };
}pressure_packet_t;

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
    uint16_t cont;          //How many bytes of the actual buffer I already recieved
    pressure_packet_t press;//packet for pressure value
    
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

