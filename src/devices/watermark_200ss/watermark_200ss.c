#include "watermark_200ss.h"
#include "application/time/time.h"
#include "drivers/acd1/acd1.h"
#include "drivers/gpio/gpio_hw.h"
#include "protocols/uart/uart.h"

#define WM_PIN_A         13
#define WM_PIN_B         14
#define WM_ADC_PIN        0
#define WM_ADC_CH         8

#define WM_RX        10000.0f
#define WM_ADC_MAX    4095.0f

#define WM_SET_US        15

#define WM_R_OPEN    35000.0f
#define WM_R_SHORT     300.0f
#define WM_R_SAT       550.0f
#define WM_CAL_F         1.0f



void watermark_init(void)
{
    gpio_PBx_analog_input_init(WM_ADC_PIN);
    polarity_PBx_off(WM_PIN_A, WM_PIN_B);
}

void watermark_sample(uint16_t *fwd, uint16_t *rev)
{
    __disable_irq();

    polarity_PBx_fwd(WM_PIN_A, WM_PIN_B);
    wm_delay_us(WM_SET_US);
    *fwd = adc1_read(WM_ADC_CH);

    polarity_PBx_rev(WM_PIN_A, WM_PIN_B);
    wm_delay_us(WM_SET_US);
    *rev = adc1_read(WM_ADC_CH);

    polarity_PBx_off(WM_PIN_A, WM_PIN_B);

    __enable_irq();
}

float watermark_resistance(uint16_t fwd, uint16_t rev)
{
    float r1 = (float)fwd / WM_ADC_MAX;
    float r2 = (float)rev / WM_ADC_MAX;

    if (r1 < 0.001f || r2 > 0.999f) return 999999.0f;

    float ra = WM_RX * (1.0f - r1) / r1;
    float rb = WM_RX * r2 / (1.0f - r2);

    return (ra + rb) / 2.0f;
}


static float wm_to_cb(float res, float tc)
{
    float resK  = res / 1000.0f;
    float tempD = 1.0f + 0.018f * (tc - 24.0f);
    float cb;

    if (res <= WM_R_SAT) return 0.0f;

    if (res > 8000.0f) {
        cb = -(-2.246f
               - 5.239f   * resK * tempD
               - 0.06756f * resK * resK * tempD * tempD) * WM_CAL_F;
    }
    else if (res > 1000.0f) {
        cb = -(-3.213f * resK - 4.093f)
             / (1.0f - 0.009733f * resK - 0.01205f * tc) * WM_CAL_F;
    }
    else {
        cb = (resK * 23.156f - 12.736f) * tempD;
    }

    if (cb < 0.0f)   cb = 0.0f;
    if (cb > 239.0f) cb = 239.0f;     

    return cb;
}

void watermark_classify(watermark_data_t *d, float soil_temp_c)
{
    if (d->r_ohm >= WM_R_OPEN || d->r_ohm == 0.0f) {
        d->status = WM_OPEN;
        d->cb     = (float)WM_CB_OPEN;
    }
    else if (d->r_ohm < WM_R_SHORT) {
        d->status = WM_SHORT;
        d->cb     = (float)WM_CB_SHORT;
    }
    else {
        d->status = WM_OK;
        d->cb     = wm_to_cb(d->r_ohm, soil_temp_c);
    }
}

watermark_data_t watermark_read(float soil_temp_c)
{
    watermark_data_t d = {0};
    float    r_sum = 0.0f;
    uint16_t f = 0, r = 0;

    for (uint8_t i = 0; i < WM_SAMPLES; i++) {
        watermark_sample(&f, &r);
        r_sum += watermark_resistance(f, r);
        delay_ms(WM_REST_MS);
    }

    d.raw_fwd = f;
    d.raw_rev = r;
    d.r_ohm   = r_sum / WM_SAMPLES;

    watermark_classify(&d, soil_temp_c);

    return d;
}

void watermark_pack_string(const watermark_data_t *d, float ntc_temp, char *out)
{
    char *p = out;

    p = pack_str(p, "cb=");
    p = pack_float1(p, d->cb);

    p = pack_str(p, ",R=");
    p = pack_int(p, (int32_t)d->r_ohm);

    p = pack_str(p, ",T=");
    if (ntc_temp >= 0.0f) *p++ = '+';
    p = pack_float1(p, ntc_temp);

    p = pack_str(p, ",st=");
    p = pack_int(p, (int32_t)d->status);

    p = pack_str(p, ",f=");
    p = pack_int(p, (int32_t)d->raw_fwd);

    p = pack_str(p, ",r=");
    p = pack_int(p, (int32_t)d->raw_rev);

    *p = '\0';
}