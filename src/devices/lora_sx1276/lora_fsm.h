#ifndef LORA_FSM_H
#define LORA_FSM_H

#include <stdint.h>

/* One packet carries system_data.data_string and the lora_tx_request()
   payload joined by LORA_TX_SEPARATOR, so it must hold both plus the
   separator and the terminator. */
#define LORA_TX_PACKET_LEN   96
#define LORA_TX_SEPARATOR    ','

/* When only one source is ready, how long the FSM waits for the other one to
   join the same packet before transmitting what it has. Must exceed the
   slowest producer's period (WM_FSM_PERIOD_MS), otherwise the window expires
   before the watermark packet arrives and the sources go out separately. */
#define LORA_TX_JOIN_MS      6000

/* Result of the last transmission requested with lora_tx_request(). */
typedef enum {
    lora_tx_idle = 0,   /* no request pending, nothing sent yet */
    lora_tx_pending,    /* request accepted, waiting for the radio */
    lora_tx_done,       /* payload was transmitted */
    lora_tx_failed      /* radio refused the payload */
} lora_tx_state_t;

#ifdef __cplusplus

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

extern "C" {
#endif

/**
 * @brief Hand a payload to the LoRa FSM for transmission.
 * The FSM is the only place the radio is driven, so every producer goes
 * through this call. The buffer is not copied: it must stay valid and
 * unchanged until lora_tx_status() leaves lora_tx_pending.
 *
 * The payload is not sent on its own: the FSM concatenates it with
 * system_data.data_string, separated by LORA_TX_SEPARATOR, so one
 * transmission carries both. If the other source is not ready, the FSM waits
 * up to LORA_TX_JOIN_MS for it and then sends whatever it has.
 *
 * @param payload Null-terminated string to transmit.
 * @return 1 if the request was accepted, 0 if a transmission is already queued
 *         or in progress.
 */
uint8_t lora_tx_request(const char *payload);

/**
 * @brief The packet last handed to the radio, i.e. the joined payloads.
 */
const char *lora_tx_packet(void);

/**
 * @brief State of the last transmission requested with lora_tx_request().
 */
lora_tx_state_t lora_tx_status(void);

/**
 * @brief Clear a finished (done/failed) transmission result back to idle.
 */
void lora_tx_clear(void);

#ifdef __cplusplus
}
#endif

#endif // LORA_FSM_H
