#ifndef LORA_SX1276_H
#define LORA_SX1276_H

#include <stdint.h>

void CS_LOW();
void CS_HIGH();

void lora_init(void);
void lora_start_rx(void);
uint8_t lora_check_rx_done(void);
void lora_send_byte(uint8_t *data, uint8_t len);

#endif
