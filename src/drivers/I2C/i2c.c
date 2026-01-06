#include "stm32f103xb.h"
#include "i2c.h"

I2C_Context i2c_ctx;

void I2C_init(void)
{

    I2C1->CR1 |= I2C_CR1_SWRST;
    I2C1->CR1 &= ~I2C_CR1_SWRST;

    I2C1->CR2 &= ~I2C_CR2_FREQ;
    I2C1->CR2 |= 8;

    I2C1->CCR = 40;

    I2C1->TRISE = 9;

    I2C1->CR1 |= I2C_CR1_PE;
}

void I2C_interrupts_init(void)
{
    I2C1->CR2 |= I2C_CR2_ITEVTEN;
    I2C1->CR2 |= I2C_CR2_ITBUFEN;
    I2C1->CR2 |= I2C_CR2_ITERREN;

    NVIC_EnableIRQ(I2C1_EV_IRQn);
    NVIC_EnableIRQ(I2C1_ER_IRQn);
}

void I2C1_EV_IRQHandler(void)
{
    I2C_Process(&i2c_ctx);
}

void I2C1_ER_IRQHandler(void)
{
    i2c_ctx.state = I2C_ERROR;
}

void I2C_Process(I2C_Context *ctx)
{

    uint32_t sr1 = I2C1->SR1;

    switch (ctx->state)
    {
    case I2C_START:
        if (sr1 & I2C_SR1_SB)
        {
            I2C1->DR = (ctx->addr << 1) | ((ctx->mode == I2C_MODE_READ) ? 1 : 0);
            ctx->state = I2C_ADDR;
        }
        break;

    case I2C_ADDR:
        if (sr1 & I2C_SR1_ADDR)
        {
            (void)I2C1->SR2;
            ctx->state = (ctx->mode == I2C_MODE_READ) ? I2C_RX : I2C_TX;
        }
        break;

    case I2C_TX:
        if (sr1 & I2C_SR1_TXE)
        {
            if (ctx->idx < ctx->len)
            {
                I2C1->DR = ctx->tx_buf[ctx->idx++];
            }
            else
            {
                I2C1->CR1 |= I2C_CR1_STOP;
                ctx->state = I2C_IDLE;
            }
        }
        break;

    case I2C_RX:
        if(sr1 & I2C_SR1_RXNE)
        {
            ctx->rx_buf[ctx->idx++] = I2C1->DR; 

            if(ctx->idx == ctx->len-1){
                I2C1->CR1 &= ~ I2C_CR1_ACK;
            }
            if(ctx->idx == ctx->len){
                I2C1->CR1 |= I2C_CR1_STOP;
                ctx->state = I2C_IDLE;

            }
        }

    break;

    case I2C_ERROR:
        I2C1->CR1 |= I2C_CR1_STOP;
        I2C1->CR1 |= I2C_CR1_ACK;
        ctx->state = I2C_IDLE;

    default:
        break;
    }
}

void I2C_StartWrite(I2C_Context *ctx,
                    uint8_t addr,
                    uint8_t *data,
                    uint8_t len)
{
    ctx->addr = addr;
    ctx->tx_buf = data;
    ctx->len = len;
    ctx->idx = 0;
    ctx->mode = I2C_MODE_WRITE;
    ctx->state = I2C_START;

    I2C1->CR1 |= I2C_CR1_START;
}

void I2C_StartRead(I2C_Context *ctx,
                    uint8_t addr,
                    uint8_t *buf,
                    uint8_t len)
{
    ctx->addr = addr;
    ctx->rx_buf = buf;
    ctx->len = len;
    ctx->idx = 0;
    ctx->mode = I2C_MODE_READ;
    ctx->state = I2C_START;

    I2C1->CR1 |= I2C_CR1_ACK;
    I2C1->CR1 |= I2C_CR1_START;
}