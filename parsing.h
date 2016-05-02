#ifndef PARSING_H
#define	PARSING_H

/******************************************************************************
 *                              TYPEDEFS                                      *
 ******************************************************************************/


//extern static inject_private_t CheckRC;
//extern static inject_private_t CheckLandDir;


extern bool PACKET_DETECTED;

/******************************************************************************
 *                          FUNCTION PROTOTYPES                               *
 ******************************************************************************/
void CheckPacketLoopInit(void);
int CheckPacketLoop(unsigned char x);

#endif	/* PARSING_H */