#ifndef PARSING_H
#define	PARSING_H

/******************************************************************************
 *                              TYPEDEFS                                      *
 ******************************************************************************/

typedef union
{
    int intwise;
    unsigned char bytewise[2];

} RC_Channel_PWM;

typedef union
{
    int intwise;
    unsigned char bytewise[2];

} Landing_Direction;

extern RC_Channel_PWM RC_Channel_6;

//extern static inject_private_t CheckRC;
//extern static inject_private_t CheckLandDir;


extern bool RC_PACKET_DETECTED;
extern bool LAND_DIR_PACKET_DETECTED;

/******************************************************************************
 *                          FUNCTION PROTOTYPES                               *
 ******************************************************************************/
void CheckRCLoopInit(void);
void CheckLandDirInit(void);
int CheckRCLoop(unsigned char x);
void CheckLandingDirection(unsigned char x);

#endif	/* PARSING_H */