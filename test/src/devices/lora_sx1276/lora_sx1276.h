#ifndef LORA_SX1276_HW_H
#define LORA_SX1276_HW_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file lora_sx1276_hw.h
 * @brief Hardware abstraction layer for LoRa SX1276 module.
 * This header file defines the hardware-specific functions for controlling the LoRa SX1276 module, 
 * including functions to control the reset pin,
 * NSS pin, and read the DIO0 pin state,
 * 
 */

void lora_init(void);
void lora_rst_high(void);
void lora_rst_low(void);
void lora_nss_low(void);
void lora_nss_high(void);
uint8_t lora_dio0_read(void);

#ifdef __cplusplus
}
#endif

#endif 