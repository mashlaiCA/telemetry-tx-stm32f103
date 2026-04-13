#include "lora_fsm.h"
#include "lora_sx1276.h"
#include "application/time/time.h"
#include "application/system_data/system_data.h"

static SX1276 *g_radio = nullptr;

typedef void (*lora_state_handler_t)(void);
lora_state_handler_t g_lora_state_handler = nullptr;

//static uint8_t g_rx[4];

static timeout_t g_timer;
int g_begin_state; //!!!

static void lora_state_init(void);
//static void lora_state_rx(void);
//static void lora_state_rx_wait(void);
//static void lora_state_check(void);
static void lora_state_tx_wait(void);
static void lora_state_tx(void);


void lora_fsm_init(SX1276 *radio)
{
    g_radio = radio;
    g_lora_state_handler = lora_state_init;
}

void lora_fsm_run(void)
{
    if (g_lora_state_handler != nullptr)
    {
        g_lora_state_handler();
    }
}

static void lora_state_init(void)
{
    int state = g_radio->begin(915.0, 125.0, 7, 5, 0x34, 17, 8, true);
    g_begin_state = state;

    if (state == RADIOLIB_ERR_NONE)
    {
        timer_set(&g_timer, 50);//new
        //g_lora_state_handler = lora_state_rx;
        g_lora_state_handler = lora_state_tx_wait;//new
    }
    else
    {
        g_lora_state_handler = nullptr;
    }
}
/*
static void lora_state_rx(void)
{
    int state = g_radio->startReceive();
    if (state == RADIOLIB_ERR_NONE)
    {
        g_lora_state_handler = lora_state_check;
        
    }
}

static void lora_state_check(void)
{
    int state = g_radio->readData(g_rx, sizeof(g_rx)-1);
    if (state == RADIOLIB_ERR_NONE)
    {
        if (g_rx[0] == '5')
        {
            timer_set(&g_timer, 50);
            g_lora_state_handler = lora_state_tx_wait;
        }
        else
        {
            g_lora_state_handler = lora_state_rx;
        }
    }
}
*/
static void lora_state_tx_wait(void)
{
    if (timer_wait(&g_timer))
    {
        timer_set(&g_timer, 1000);
       // system_data.lora_busy = 0;//
        //system_data.data_string_ready = 0;//
        g_lora_state_handler = lora_state_tx;
    }
}

static void lora_state_tx(void){
   // if(system_data.data_string_ready && !system_data.lora_busy){//
       // system_data.lora_busy = 1;//
        //sensor_update(&system_data, 2);
       // build_payload(&system_data);
    int state = g_radio->transmit("32,23,23,23,23");//system_data.data_string);//
    

    if(state == RADIOLIB_ERR_NONE){
       // timer_set(&g_timer, 50);
       // g_lora_state_handler = lora_state_rx_wait;
       g_lora_state_handler = lora_state_tx_wait;//new
    }
//}
}
/*
static void lora_state_rx_wait(void)
{
    if (timer_wait(&g_timer))
    {
        g_lora_state_handler = lora_state_rx;
    }
}
*/