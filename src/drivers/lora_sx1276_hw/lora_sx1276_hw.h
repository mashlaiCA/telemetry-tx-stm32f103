/**
 * @file  lora_sx1276_hw.h
 * @brief Direct SX1276 register access over SPI1, bypassing RadioLib (debugging helpers).
 * This file provides:
 * 1. sx_write() - write one register.
 * 2. sx_read() - read one register.
 * NOTE: the include guard LORA_SX1276_HW_H is also used by devices/lora_sx1276/lora_sx1276.h,
 * so the two headers cannot both be included in one translation unit.
 */

#ifndef LORA_SX1276_HW_H
#define LORA_SX1276_HW_H

#include <stdint.h>


void sx_write(uint8_t addr, uint8_t data); // Writes data to SX1276 register addr (0x00..0x7F)
uint8_t sx_read(uint8_t addr);             // Returns the value of SX1276 register addr (0x00..0x7F)


#endif