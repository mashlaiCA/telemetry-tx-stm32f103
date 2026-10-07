/**
 * @file  lora_sx1276.h
 * @brief SX1276 control-pin access used by the RadioLib HAL.
 * This file provides:
 * 1. lora_init() - configures the RST (PB5) and DIO0 (PB1) pins.
 * 2. RST control (PB5), NSS control (PB12) and DIO0 read (PB1).
 * NOTE: the include guard LORA_SX1276_HW_H is shared with
 * drivers/lora_sx1276_hw/lora_sx1276_hw.h, so the two headers cannot both be
 * included in one translation unit.
 */

#ifndef LORA_SX1276_HW_H
#define LORA_SX1276_HW_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void lora_init(void);          // PB5 RST -> push-pull output, PB1 DIO0 -> input with pull-down
void lora_rst_high(void);      // Release SX1276 reset (PB5 = 1), then 10 ms delay
void lora_rst_low(void);       // Assert SX1276 reset (PB5 = 0), then 10 ms delay
void lora_nss_low(void);       // Select SX1276 on SPI (PB12 = 0)
void lora_nss_high(void);      // Deselect SX1276 (PB12 = 1)
uint8_t lora_dio0_read(void);  // Level of DIO0 (PB1): 1 = high, 0 = low

#ifdef __cplusplus
}
#endif

#endif