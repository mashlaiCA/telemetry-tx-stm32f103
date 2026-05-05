#ifndef LORA_FSM_H
#define LORA_FSM_H

#include <RadioLib.h>

typedef void (*lora_state_handler_t)(void);
extern lora_state_handler_t g_lora_state_handler;


extern int statusTransmit;
extern int statusTXdone;
extern int finish;

extern int g_begin_state; 
void onTxDone(void);
void lora_fsm_init(SX1276* radio);
void lora_fsm_run(void);


#endif // LORA_FSM_H 