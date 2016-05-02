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

#define X25_INIT_CRC        0xffff
#define X25_VALIDATE_CRC    0xf0b8

/******************************************************************************/
/* Local Variables                                                            */
/******************************************************************************/
/**
 * Local variable. Stores all the local data inject module uses
 */
static inject_private_t this;

/**
 * Sample pressure packet used for testing. We should take this array as
 * base and edit only the required fields
 */
static const uint8_t __WP_CLEAR_SAMPLE [WP_CLEAR_FULL_LEN] =
    {0xfe, 0x04, 0x78, 0xff, 0xbe, 0x2d, 0x04, 0x00, 0x01, 0x01, 0xa8, 0x4e };

static const uint8_t MAVLINK_MESSAGE_CRCS[] =
    {50, 124, 137, 0, 237, 217, 104, 119, 0, 0, 0, 89, 0, 0, 0, 0, 0, 0, 0, 0,
    214, 159, 220, 168, 24, 23, 170, 144, 67, 115, 39, 246, 185, 104, 237, 244,
    222, 212, 9, 254, 230, 28, 28, 132, 221, 232, 11, 153, 41, 39, 78, 0, 0, 0,
    15, 3, 0, 0, 0, 0, 0, 153, 183, 51, 82, 118, 148, 21, 0, 243, 124, 0, 0, 38,
    20, 158, 152, 143, 0, 0, 0, 106, 49, 22, 143, 140, 5, 150, 0, 231, 183, 63,
    54, 0, 0, 0, 0, 0, 0, 0, 175, 102, 158, 208, 56, 93, 138, 108, 32, 185, 84,
    34, 0, 124, 237, 4, 76, 128, 56, 116, 134, 237, 203, 250, 87, 203, 220, 25,
    226, 46, 29, 223, 85, 6, 229, 203, 1, 195, 109, 168, 181, 0, 0, 0, 0, 0, 0,
    154, 178, 0, 134, 219, 208, 188, 84, 22, 19, 21, 134, 0, 78, 68, 189, 127,
    154, 21, 21, 144, 1, 234, 73, 181, 22, 83, 167, 138, 234, 240, 47, 189, 52,
    174, 229, 85, 0, 0, 72, 0, 0, 0, 0, 0, 0, 71, 0, 0, 0, 0, 0, 0, 134, 205, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 204, 49, 170,
    44, 83, 46, 0};

/******************************************************************************/
/* Public Variables                                                           */
/******************************************************************************/
inject_t inject;


/******************************************************************************/
/* Local Functions                                                            */
/******************************************************************************/


static uint16_t crc_accumulate(uint8_t b, uint16_t crc)
{
    
    uint8_t ch = (uint8_t)(b ^ (uint8_t)(crc & 0x00ff));
    ch = (uint8_t)(ch ^ (ch << 4));
    return (uint16_t)((crc >> 8) ^ (ch << 8) ^ (ch << 3) ^ (ch >> 4));

}

static uint16_t crc_calculate(uint8_t pBuffer[], uint8_t length)
{
    if (length < 1)
    {
        return 0xffff;
    }
    // For a "message" of length bytes contained in the unsigned char array
    // pointed to by pBuffer, calculate the CRC
    // crcCalculate(unsigned char* pBuffer, int length, unsigned short* checkConst) < not needed

    uint16_t crcTmp;
    int i;

    crcTmp = X25_INIT_CRC;

    for (i = 1; i < length; i++) // skips header
    {
        crcTmp = crc_accumulate(pBuffer[i], crcTmp);
        //Console.WriteLine(crcTmp + " " + pBuffer[i] + " " + length);
    }

    return (crcTmp);
}

/******************************************************************************/
/* Public Functions                                                           */
/******************************************************************************/

/**
 * Init inject module
 */
void InjectInit()
{
    //Init state
    this.state = 0;
    inject.timer = INJECT_PERIOD;
    CircularBufferInit(&(inject.outBuff), this.bufArray, RADIO_OUTPUT_BUFFER);

    //Init injection buffer
    memcpy(this.packet.buf, __WP_CLEAR_SAMPLE, WP_CLEAR_FULL_LEN);
    this.packet.count = 0;
    this.packet.sys_id = WP_CLEAR_SYSTEM_ID;
    this.packet.comp_id = WP_CLEAR_COMP_ID;
}

/**
 * Analyze a stream byte by byte to detect packets and try to send custom packets in response
 * @param rx - next byte of the stream
 */
void InjectLoop(uint8_t rx)
{
    if(this.state > MACHINE_NAME_STATE_MAX)
    {
        InjectInit();
        return;
    }
    
    //Add byte to the output buffer if there is free space
    if(!CircularBufferIsFull(&(inject.outBuff)))
        CircularBufferEnque(&(inject.outBuff), rx);
    else
        return;

    //Analyze received byte
    switch (this.state)
    {
        case INJECT_STATE_MSG_END:              //current state is the end of previous message, meaning we expect to see beginning of next message
            if(rx == 0xFE)                      //0xFE is the universal start byte
            {
                //First byte of the packet detected
                this.state = INJECT_STATE_WAIT_LEN;     //we switch states to analyze the length byte
            }
            break;

        case INJECT_STATE_WAIT_LEN:
            this.cont = 2;                    //Already has 2 bytes (start byte and length byte)
            this.fullLen = rx + 6 + 2 + 8;    //Calculate the total packet length based on MAVLink structure
            this.state = INJECT_STATE_WAIT_END;
            break;

        case INJECT_STATE_WAIT_END:
            if(++this.cont >= this.fullLen)
            {
                this.state = INJECT_STATE_MSG_END;  //Full packet received
            }
            break;

        case MACHINE_NAME_STATE_MAX:
        default:
            //Error, unknown state
            InjectInit();
            break;            
    }
}

/**
 * If it's time to inject and there are space in the buffer,
 * prepare the message and inject it
 */
void InjectTryInject()
{   
    uint8_t i;
    uint16_t cheksum;

    //Restart timer
    inject.timer = INJECT_PERIOD;

    //Set Data
    //This specific message does not have actual data in the payload. It is all zeros.
    uint8_t data = 0;
    //

    //Update values
    memcpy(this.packet.data, &data , sizeof(float));
    this.packet.count++;

    //calculate checksum
    cheksum = crc_calculate(this.packet.buf, WP_CLEAR_MSG_LEN+6);
    cheksum = crc_accumulate(
            MAVLINK_MESSAGE_CRCS[this.packet.msgType],
            cheksum);

    //copy the proper checksum to the packet structure
    memcpy(this.packet.checksum, &cheksum, sizeof(cheksum));

    //Send the packet byte by byte
    for(i=0; i<WP_CLEAR_FULL_LEN; i++)
        CircularBufferEnque(&(inject.outBuff), this.packet.buf[i]);
}

