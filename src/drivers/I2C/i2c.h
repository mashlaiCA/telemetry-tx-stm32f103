#ifndef I2C_H
#define I2C_H

#include <stdint.h>

typedef enum {
    I2C_IDLE, // idle state, no I2C activity 
    I2C_START, // start condition generated
    I2C_ADDR, // slave address transmission phase
    I2C_TX, //data transmission phase
    I2C_RX, // data reseption phase
    I2C_STOP, // stop condition generated
    I2C_ERROR, //error state handling
}I2C_State;

typedef enum {
I2C_MODE_WRITE,
I2C_MODE_READ
} I2C_MODE;


typedef struct{
    I2C_State state; // current state of the I2C finite state machine
    I2C_MODE mode;
    uint8_t addr; // I2C slave address (7 bit address + R/W handled separately)
    uint8_t *tx_buf; // pointer to transmission buffer
    uint8_t *rx_buf; // poiner to reception buffer
    uint8_t len; // total number of bytes to transmit or receive
    uint8_t idx; // Current byte index int the buffer
}I2C_Context;

extern I2C_Context i2c_ctx;

void I2C_init(void);

void I2C_interrupts_init(void);

void I2C1_EV_IRQHandler(void);

void I2C1_ER_IRQHandler(void);

void I2C_Process(I2C_Context *ctx);

void I2C_StartWrite(I2C_Context *ctx,
                    uint8_t addr,
                    uint8_t *data,
                    uint8_t len);

void I2C_StartRead(I2C_Context *ctx,
                    uint8_t addr,
                    uint8_t *buf,
                    uint8_t len);


#endif
