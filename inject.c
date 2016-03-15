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
 * base and edit pnly the required fields
 */
static const uint8_t __PRESSURE_SAMPLE[PARAM_UPDATE_FULL_LEN] =
    {0xfe, 0x17, 0xfd, 0xff, 0xbe, 0x17, 0x00, 0x50, 0xc3, 0x47, 0x01, 0x01,
     0x47, 0x4e, 0x44, 0x5f, 0x41, 0x42, 0x53, 0x5f, 0x50, 0x52, 0x45, 0x53,
     0x53, 0x00, 0x00, 0x00, 0x09, 0x44, 0xac };

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

    //Init Presure ijection buffer
    memcpy(this.press.buf, __PRESSURE_SAMPLE, PARAM_UPDATE_FULL_LEN);
    this.press.count = 0;
    this.press.sys_id = PRESS_SYSTEM_ID;
    this.press.comp_id = PRESS_COMP_ID;
}

/**
 * Analice a stream byte by byte to detect packets and try to inject custom 
 * packets into the stream
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

    //Analyze recieved byte
    switch (this.state)
    {
        case INJECT_STATE_MSG_END:
            if(rx == 0xFE)
            {
                //First byte of the packet detected
                this.state = INJECT_STATE_WAIT_LEN;
            }
            break;

        case INJECT_STATE_WAIT_LEN:
            //Len byte recieved
            this.cont = 2;                    //Already has 2 bytes
            this.fullLen = rx + 6 + 2 + 8;    //Calc the packet len
            this.state = INJECT_STATE_WAIT_END;
            break;

        case INJECT_STATE_WAIT_END:
            if(++this.cont >= this.fullLen)
            {
                //Full packet recieved
                this.state = INJECT_STATE_MSG_END;
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

    //I can't inject here?
    if(this.state != INJECT_STATE_MSG_END)
        return;

    //It isn't time to inject
    if(inject.timer)
        return;

    //Is there enounhg space to inject?
    if(CircularBufferFreeSpace(&(inject.outBuff)) > (2*PARAM_UPDATE_FULL_LEN))
    {
        uint8_t i;
        uint16_t cheksum;
        float pres = 10.0f;

        //Restart timer
        inject.timer = INJECT_PERIOD;

        //Get pressure
        Bar_Calculate();
        pres = (float)bar.pres;

        //Update values
        memcpy(this.press.data, &pres, sizeof(float));
        this.press.count++;

        //checksum
        cheksum = crc_calculate(this.press.buf, PARAM_UPDATE_MSG_LEN+6);
        cheksum = crc_accumulate(
                MAVLINK_MESSAGE_CRCS[this.press.msgType],
                cheksum);

        memcpy(this.press.checksum, &cheksum, sizeof(cheksum));

        //Inject
        for(i=0; i<PARAM_UPDATE_FULL_LEN; i++)
            CircularBufferEnque(&(inject.outBuff), this.press.buf[i]);


        {
            char buff[50];
            static uint16_t cont = 0;
            sprintf(buff, "\n\rInjected = %d     ", cont++);
            UART1_WriteBuffer((uint8_t*)buff, strlen(buff));
        }
    }
}

