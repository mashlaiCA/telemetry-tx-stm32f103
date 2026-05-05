#include <RadioLib.h>
#include "lora_fsm.h"
#include "lora_sx1276.h"
#include "application/time/time.h"
#include "application/system_data/system_data.h"
#include "radiolib_stm32_hal/radiolib_stm32_hal.h"

static SX1276 *g_radio = nullptr;

typedef void (*lora_state_handler_t)(void);
lora_state_handler_t g_lora_state_handler = nullptr;

volatile uint8_t lora_tx_done_flag = 0; // Flag to indicate that transmission is done, set in onTxDone() ISR

int statusTransmit = 999; //

int finish = 66; // Variable to hold the result of finishTransmit() for debugging purposes

int statusTXdone;

static timeout_t g_timer;
int g_begin_state;

static void lora_state_init(void);
// static void lora_state_rx(void);
// static void lora_state_rx_wait(void);
// static void lora_state_check(void);
static void lora_state_tx_done(void);
static void lora_state_tx_wait(void);
static void lora_state_tx(void);

void onTxDone(void)
{
    lora_tx_done_flag = 1;
}

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

    if (state == RADIOLIB_ERR_NONE)
    {

        g_radio->setDio0Action(onTxDone, 0);
        // g_lora_state_handler = lora_state_rx;
        g_lora_state_handler = lora_state_tx_wait; // new
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
    if (system_data.ready_data_creation_flag == 1 && system_data.lora_busy == 0)
    {
        if (hal.digitalRead(1) == 0)
        {
            g_lora_state_handler = lora_state_tx;
            system_data.lora_busy = 1;
        }
    }
}
static void lora_state_tx(void)
{

    lora_tx_done_flag = 0;

    int state = g_radio->startTransmit(system_data.data_string);
    statusTransmit = state;
    if (state == RADIOLIB_ERR_NONE)
    {

        g_lora_state_handler = lora_state_tx_done;
    }
    else
    {

        system_data.lora_busy = 0;
        g_lora_state_handler = lora_state_tx_wait;
    }
}

static void lora_state_tx_done(void)
{
    if (lora_tx_done_flag)
    {
        statusTXdone ++;

        int finishState = g_radio->finishTransmit();
        finish = finishState; 

        lora_tx_done_flag = 0;

        g_radio->standby();

        system_data.lora_busy = 0;
        system_data.ready_data_creation_flag = 0;
        system_data.ready_sensors_flag = 0;

        g_lora_state_handler = lora_state_tx_wait;
    }
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
