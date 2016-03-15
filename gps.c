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
#include <stdio.h>
#include <string.h>
#include "mcc_generated_files/uart4.h"
#include "gps.h"
#include "debug.h"
#include "utests/seatest.h"
#include "./utests/unit.h"
#include "power.h"


/******************************************************************************/
/* Variable                                                                   */
/******************************************************************************/

static gps_private_data_t gpsPrData;
gps_public_data_t gps;



/******************************************************************************/
/* Private Functions                                                          */
/******************************************************************************/

/**
 * Clear buffer for GPS NMEA commands
 */
static void gps_ClearBuffer()
{
    memset(gpsPrData.buf, 0, GPS_BUF_MAX);
    gpsPrData.len = 0;
}


/**
 * Take a field from GPS message and decode it
 * @param s Field string
 * @param fNo Field number
 */
static void gps_TakeField(uint8_t *s, uint8_t fNo)
{
    //If the location is invalid, I'm not considering the other fields
    if(fNo > 2 && gps.locState == GPS_LOC_INVALID)
        return;

    switch(fNo)
    {
        case 2:
            if(s[0] == 'A')
                gps.locState = GPS_LOC_VALID;
            else
            {
                gps.locState= GPS_LOC_INVALID;
                strcpy(gps.latitude, "Invalid");
                strcpy(gps.longitude, "Location");
            }
        break;                                                    //Data valid?

        case 3:
            memcpy(&(gps.latitude[1]), &(s[0]), 2);
            memcpy(&(gps.latitude[4]), &(s[2]), 6);
            break;

        case 4:
            if(s[0] == 'S')                     //N/S ?
                gps.latitude[0] = '-';
            break;

        case 5:
            memcpy(gps.longitude+1, s+0, 3);
            memcpy(gps.longitude+5, s+3, 6);
            break;

        case 6:
            if(s[0] == 'W')                     //W/E ?
                gps.longitude[0] = '-';
            break;

        default:
            break;
    }
}

/**
 * Decodes the actual GPS frame
 */
static void gps_Decode()
{
    uint8_t i;          //Array index
    uint8_t fNo=0;      //Actual field number

    //Separe fields in strings
    for(i=0; gpsPrData.buf[i]; i++)
        if(gpsPrData.buf[i] == ',' || gpsPrData.buf[i] == '*')
            gpsPrData.buf[i] = 0;

    //Clear loc variables
    memset(gps.latitude,  0, GPS_LOC_LEN);
    memset(gps.longitude, 0, GPS_LOC_LEN);
    memset(gps.latitude,  ' ', 8);
    memset(gps.longitude, ' ', 8);

    //Take data field by field
    for(fNo=0, i=0; fNo < 10; fNo++)
    {
        gps_TakeField(&(gpsPrData.buf[i]), fNo);      //Take actual field
        while(gpsPrData.buf[i++]);                    //Go to the end of the field
    }

    //Indicated values are updated!
    gpsPrData.updated = true;
}

/**
 * Run recieve machine reading frames from GPS 
 */
static void gps_next_char(uint8_t rx)
{
    //Store new byte to buffer
    gpsPrData.buf[gpsPrData.len++] = rx;

    //Buffer is full!
    if(gpsPrData.len >= (GPS_BUF_MAX-2))
    {
        gps_ClearBuffer();
        return;
    }


    switch (gpsPrData.state)
    {
        case GPS_STATE_WAIT_START:
            if(rx == '$')
            {
                gps_ClearBuffer();
                gpsPrData.state = GPS_STATE_WAIT_COMMA;
            }
            break;

        case GPS_STATE_WAIT_COMMA:
            if(rx == ',')
            {
                if(strstr((char*)gpsPrData.buf, "GPRMC,") != 0)
                {           
                    //String match, we ca go next state
                    gpsPrData.state = GPS_STATE_WAIT_END;
                }
                else
                {
                    //String not match, lets wait for next frame and discart this one
                    gps_ClearBuffer();
                    gpsPrData.state = GPS_STATE_WAIT_START;
                }
            }
            break;

        case GPS_STATE_WAIT_END:
            if(rx == ASCII_LF || rx == ASCII_CR)
            {
                gps_Decode();
                gps_ClearBuffer();
                gpsPrData.state = GPS_STATE_WAIT_START;
            }
            break;

        case GPS_STATE_MAX:
        default:
            //Error, unknown state
            gps_Init();
    }
}



/******************************************************************************/
/* Public Functions                                                           */
/******************************************************************************/

/**
 * Init the module and variables
 */
void gps_Init()
{
    gpsPrData.state = 0;                //Init state
    gpsPrData.updated = false;          //Data are not updated!
    gpsPrData.onFlag = false;           //Start sequence not initialiced

    gps.locState = GPS_LOC_UNKNOWN;
    strcpy(gps.latitude, "Unknown");
    strcpy(gps.longitude, "Location");

    gps_ClearBuffer();              //Clear buffer

}


/**
 * Turns ON GPS module with delay
 * @param delay - msSeconds to delay before turning ON GPS
 */
void gps_PowerOn(uint16_t delay)
{
    gpsPrData.onFlag = true;
    gps.onTimer = delay;
}

/**
 * Check if there is a new data from GPS UART
 * If so, send the new byte to gps_next_char routine
 */
void gps_Routine()
{
    //GPS off?
    if(gpsPrData.onFlag && gps.onTimer == 0)
    {
        POWER_GPS_COMPASS_TURN_ON();
    }
    else
        return;

    //No new data
    if(UART4_ReceiveBufferIsEmpty())
        return;

    gps_next_char(UART4_Read());
}

/**
 * Dterminates if data are updated and clear the update flag. User should
 * new data inmidiatly after calling this routine if new data are there
 * @return if true, there are new data to read.
 */
bool gps_IsLocationUpdated()
{
    if(gpsPrData.updated)
    {
        gpsPrData.updated = false;
        return true;
    }
    else
        return false;
}

/*
 * Prints last data on debug terminal
 */
void __gps_TestPrintResults(bool newLine)
{
    char buf[128];
    
    sprintf(buf, "%s %s %s",
            gps.latitude,
            gps.longitude,
            newLine ? "\n\r" : "");

    __debug_print(buf);
}


/******************************************************************************
 *                  Unit Tests Section                                        *
 ******************************************************************************/

#ifdef __RUN_UNIT_TESTS__

static void __gps_UnitTest_Complete()
{
    char *s = "19:56:50  $GPRMC,195650.00,A,4104.21583,N,08131.68109,W,1.350,,220115,,,A*6F\n\r";
    uint16_t i;

    //Init gps module
    gps_Init();

    //Test initial values
    assert_int_equal(GPS_LOC_UNKNOWN, gps.locState);
    assert_string_equal("Unknown", gps.latitude);
    assert_string_equal("Location", gps.longitude);

    //Process the string
    for(i=0; s[i]; i++)
        gps_next_char(s[i]);


    //Test result coordinates
    assert_string_equal(" 41 04.215", gps.latitude);
    assert_string_equal("-081 31.681", gps.longitude);
    assert_true(gpsPrData.updated);
    assert_true(gps_IsLocationUpdated());
    assert_false(gpsPrData.updated);
    assert_int_equal(GPS_LOC_VALID, gps.locState);
}

static void __gps_UnitTest_Locations()
{
    char *s;
    uint16_t i;

    //Init gps module
    gps_Init();

    //test NW
    s = "19:56:50  $GPRMC,195650.00,A,4104.21583,N,08131.68109,W,1.350,,220115,,,A*6F\n\r";
    for(i=0; s[i]; i++) gps_next_char(s[i]);
    assert_string_equal(" 41 04.215", gps.latitude);
    assert_string_equal("-081 31.681", gps.longitude);

    //test SE
    s = "19:56:50  $GPRMC,195650.00,A,4104.21583,S,08131.68109,E,1.350,,220115,,,A*6F\n\r";
    for(i=0; s[i]; i++) gps_next_char(s[i]);
    assert_string_equal("-41 04.215", gps.latitude);
    assert_string_equal(" 081 31.681", gps.longitude);

    //test invalid data
    s = "19:56:50  $GPRMC,195650.00,N,4204.21583,S,08231.68109,E,1.350,,220115,,,A*6F\n\r";
    for(i=0; s[i]; i++) gps_next_char(s[i]);
    assert_string_doesnt_contain("-41 04.215", gps.latitude);
    assert_string_doesnt_contain(" 081 31.681", gps.longitude);
    assert_int_equal(GPS_LOC_INVALID, gps.locState);
}

static void __gps_UnitTest_VoidSring()
{
    char *s = "??:??:??  $GPRMC,,V,,,,,,,,,,N*53\n\r";
    uint16_t i;

    //Init gps module
    gps_Init();

    //Process the string
    for(i=0; s[i]; i++)
        gps_next_char(s[i]);


    //Test result coordinates
    assert_string_equal("Invalid", gps.latitude);
    assert_string_equal("Location", gps.longitude);
    assert_true(gpsPrData.updated);
    assert_true(gps_IsLocationUpdated());
    assert_false(gpsPrData.updated);
    assert_int_equal(GPS_LOC_INVALID, gps.locState);
}

void __gps_UnitTest_Fixture( void )
{
    test_fixture_start();
    run_test(__gps_UnitTest_Complete);
    run_test(__gps_UnitTest_Locations);
    run_test(__gps_UnitTest_VoidSring);
    test_fixture_end();
}


#endif

