/******************************************************************************/
/* TypeDefs                                                                   */
/******************************************************************************/
typedef enum
{
	RELAY_STATE_INIT = 0,    	// <-- Init state is 0
    RELAY_STATE_RUNNING,
	RELAY_STATE_ERROR
}relay_states_t;

typedef struct
{
    relay_states_t state;   
}relay_data_t;

/******************************************************************************/
/* Variable                                                                   */
/******************************************************************************/

extern relay_data_t relayData;


/******************************************************************************/
/* Functions                                                                  */
/******************************************************************************/

void RelayInit();
void RelayLoop();
void relayFromRadio();
void relayResetInjector();

