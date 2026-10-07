#include "watermark_200ss.h"
#include "application/time/time.h"
#include "drivers/acd1/acd1.h"
#include "drivers/gpio/gpio_hw.h"
#include "protocols/uart/uart.h"

#define WM_PIN_A         13 // PB13: probe drive A
#define WM_PIN_B         14 // PB14: probe drive B
#define WM_ADC_PIN        0 // PB0: divider midpoint
#define WM_ADC_CH         8 // ADC1 channel 8 = PB0

#define WM_RX        10000.0f // Reference resistor of the divider, Ohm
#define WM_ADC_MAX    4095.0f // 12-bit full scale

#define WM_SET_US        15 // Settling time after applying the excitation, us

#define WM_R_OPEN    35000.0f // R >= 35 kOhm -> probe open / disconnected
#define WM_R_SHORT     300.0f // R < 300 Ohm -> probe shorted
#define WM_R_SAT       550.0f // R <= 550 Ohm -> saturated soil, cb = 0
#define WM_CAL_F         1.0f // Calibration factor for the two upper ranges (1.0 = none)



/* PB0 as analog input, drive pins released. */
void watermark_init(void)
{
    gpio_PBx_analog_input_init(WM_ADC_PIN); // PB0 analog
    polarity_PBx_off(WM_PIN_A, WM_PIN_B);   // PB13/PB14 floating: no current through the probe
}

/* One forward + reverse excitation (see watermark_200ss.h). */
uint8_t watermark_sample(uint16_t *fwd, uint16_t *rev)
{
    uint16_t f; // Forward ADC code
    uint16_t r; // Reverse ADC code
    uint8_t  ok; // 1 = conversion succeeded



    /* Interrupts are disabled so an ISR cannot lengthen one direction only:
       unequal excitation times mean a net DC current through the electrodes,
       which degrades the probe electrochemically. The section takes about
       2 * (15 us + 63 us), far below the 1 ms TIM2 period, so no tick is lost.
       PRIMASK is saved and restored so a caller's critical section is kept. */
    uint32_t primask = __get_PRIMASK();
    __disable_irq();

    /* --- FORWARD direction --- */
    polarity_PBx_fwd(WM_PIN_A, WM_PIN_B); // PB13 = 1, PB14 = 0
    wm_delay_us(WM_SET_US);               // Settling; TIM2->CNT based, works with interrupts off
    f = adc1_read(WM_ADC_CH);             // Forward reading
    polarity_PBx_off(WM_PIN_A, WM_PIN_B); // Remove excitation before checking the result
    ok = adc1_last_error ? 0u : 1u;


    if (!ok)
    {
        __set_PRIMASK(primask); // Restore interrupt state; excitation is already off
        return 0;
    }

    /* --- REVERSE direction --- */
    polarity_PBx_rev(WM_PIN_A, WM_PIN_B); // PB13 = 0, PB14 = 1
    wm_delay_us(WM_SET_US);               // Same settling time as forward
    r = adc1_read(WM_ADC_CH);             // Reverse reading
    polarity_PBx_off(WM_PIN_A, WM_PIN_B); // Remove excitation
    ok = adc1_last_error ? 0u : 1u;

    __set_PRIMASK(primask); // Restore interrupt state

    if (!ok)
    {
        return 0; // Reverse conversion failed
    }

    *fwd = f;
    *rev = r;

    return 1;
}

/* Probe resistance from one sample, averaged over both directions.
   Forward: code/4095 = Rx / (Rx + Rs)  ->  Rs = Rx * (1 - r1) / r1.
   Reverse: code/4095 = Rs / (Rx + Rs)  ->  Rs = Rx * r2 / (1 - r2). */
float watermark_resistance(uint16_t fwd, uint16_t rev)
{
    float r1 = (float)fwd / WM_ADC_MAX; // Forward ratio
    float r2 = (float)rev / WM_ADC_MAX; // Reverse ratio

    if (r1 < 0.001f || r2 > 0.999f) return 999999.0f; // Rail value: division would blow up; treated as OPEN

    float ra = WM_RX * (1.0f - r1) / r1; // Resistance from the forward reading
    float rb = WM_RX * r2 / (1.0f - r2); // Resistance from the reverse reading

    return (ra + rb) / 2.0f; // Average of both directions
}


/* Resistance [Ohm] + soil temperature [degC] -> soil water tension [cb].
   Piecewise conversion with temperature correction tempD = 1 + 0.018 * (T - 24),
   T clamped to 0..60 degC:
     R <= 550 Ohm          : 0 (saturated);
     550 < R <= 1 kOhm     : linear segment;
     1 kOhm < R <= 8 kOhm  : rational segment with T in the denominator;
     R > 8 kOhm            : quadratic segment in R * tempD.
   TODO(comment): the source of the coefficients is not referenced in the code. */
static float wm_to_cb(float res, float tc)
{
    if (tc < 0.0f)   tc = 0.0f;  // Clamp the compensation temperature
    if (tc > 60.0f)  tc = 60.0f;

    float resK  = res / 1000.0f;               // Resistance in kOhm
    float tempD = 1.0f + 0.018f * (tc - 24.0f); // Temperature correction, 1.0 at 24 degC
    float cb;

    if (res <= WM_R_SAT) return 0.0f; // Saturated soil

    if (res > 8000.0f) {
        cb = -(-2.246f
               - 5.239f   * resK * tempD
               - 0.06756f * resK * resK * tempD * tempD) * WM_CAL_F; // Dry range
    }
    else if (res > 1000.0f) {
        float denom = 1.0f - 0.009733f * resK - 0.01205f * tc;

        if (denom < 0.05f) {
            return 0.0f; // Division guard; with R <= 8 kOhm and T <= 60 degC denom >= 0.199, so not reached in practice
        }

        cb = -(-3.213f * resK - 4.093f) / denom * WM_CAL_F; // Middle range
    }
    else {
        cb = (resK * 23.156f - 12.736f) * tempD; // Wet range, 550 Ohm..1 kOhm
    }

    if (cb < 0.0f)   cb = 0.0f;
    if (cb > 239.0f) cb = 239.0f; // 240 and 255 are reserved for SHORT / OPEN

    return cb;
}

/* Status and cb from the averaged resistance. */
void watermark_classify(watermark_data_t *d, float soil_temp_c)
{
    if (d->r_ohm >= WM_R_OPEN || d->r_ohm == 0.0f) { // Open probe, or no valid sample (r_ohm = 0)
        d->status = WM_OPEN;
        d->cb     = (float)WM_CB_OPEN;
    }
    else if (d->r_ohm < WM_R_SHORT) { // Shorted probe
        d->status = WM_SHORT;
        d->cb     = (float)WM_CB_SHORT;
    }
    else {
        d->status = WM_OK;
        d->cb     = wm_to_cb(d->r_ohm, soil_temp_c); // 0..239 cb
    }
}

/* Blocking measurement: WM_SAMPLES samples with WM_REST_MS pauses, averaged and classified. */
watermark_data_t watermark_read(float soil_temp_c)
{
    watermark_data_t d = {0};
    float    r_sum = 0.0f;   // Sum of resistances of successful samples
    uint16_t f = 0, r = 0;   // Raw codes of the last successful sample

    uint8_t taken = 0; //count only successful samples

    for (uint8_t i = 0; i < WM_SAMPLES; i++) {
        if (!watermark_sample(&f, &r)) {
            continue; // ADC failure - discard sample, excitation already removed
        }
        r_sum += watermark_resistance(f, r);
        taken++;
        delay_ms(WM_REST_MS); // Depolarization pause (blocking)
    }

    d.raw_fwd = f;
    d.raw_rev = r;
    /* Divide by the number of samples ACTUALLY taken, not by WM_SAMPLES:
       otherwise one discarded sample would understate the average resistance
       by a third and dry soil could look wet. With no successful sample r_ohm
       is 0, which watermark_classify() reports as OPEN. */
    d.r_ohm   = taken ? (r_sum / (float)taken) : 0.0f;

    watermark_classify(&d, soil_temp_c);

    return d;
}

/* Debug text "cb=<x.x>,R=<ohm>,T=<+x.x>,st=<n>,f=<raw>,r=<raw>" (max ~47 chars + '\0'). */
void watermark_pack_string(const watermark_data_t *d, float ntc_temp, char *out)
{
    char *p = out;

    p = pack_str(p, "cb=");
    p = pack_float1(p, d->cb); // Tension, one decimal

    p = pack_str(p, ",R=");
    p = pack_int(p, (int32_t)d->r_ohm); // Resistance, whole Ohm

    p = pack_str(p, ",T=");
    if (ntc_temp >= 0.0f) *p++ = '+'; // Explicit sign for non-negative temperatures
    p = pack_float1(p, ntc_temp);    // Soil temperature, one decimal

    p = pack_str(p, ",st=");
    p = pack_int(p, (int32_t)d->status); // wm_status_t as a number

    p = pack_str(p, ",f=");
    p = pack_int(p, (int32_t)d->raw_fwd);

    p = pack_str(p, ",r=");
    p = pack_int(p, (int32_t)d->raw_rev);

    *p = '\0';
}