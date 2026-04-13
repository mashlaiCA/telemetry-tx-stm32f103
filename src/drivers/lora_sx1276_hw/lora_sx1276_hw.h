#ifndef LORA_SX1276_HW_H
#define LORA_SX1276_HW_H

#include <stdint.h>


void sx_write(uint8_t addr, uint8_t data);
uint8_t sx_read(uint8_t addr);


#endif 