#include "ntc.h"
#include "math.h"
#include "drivers/gpio/gpio_hw.h"
#include "drivers/acd1/acd1.h"

/* Divider: R_FIXED from VCC to the ADC node, NTC from the ADC node to GND,
   so V_adc / VCC = R_ntc / (R_FIXED + R_ntc). */
#define R_FIXED 10000.0f   // Fixed divider resistor, Ohm
#define R_NOMINAL 10000.0f // NTC resistance at T_NOMINAL, Ohm
#define T_NOMINAL 298.15f  // 25 degC in Kelvin
#define B_COEFF 3950.0f    // NTC Beta coefficient, K
#define ADC_MAX 4095.0f    // 12-bit full scale

/* Configures the NTC input pin. */
void ntc_init(void)
{
    ntc_gpio_init(); // PA1 analog input
}

/* Limits of a valid raw ADC code.
   At raw = 0 (short) or raw = 4095 (open) the divider degenerates and the
   formula divides by zero. A margin of a few counts on each side also cuts
   off obviously unrealistic values. */
#define NTC_RAW_MIN     10u
#define NTC_RAW_MAX   4085u

/* Temperature returned for a faulty thermistor. 24 C is the reference point
   of the Watermark formula (tempD == 1), so the compensation becomes neutral
   instead of distorting the result. */
#define NTC_FALLBACK_C  24.0f

/* Raw ADC code -> degC using the Beta equation 1/T = 1/T0 + ln(R/R0)/B. */
static float ntc_calc_temperature(uint16_t raw_adc)
{
    /* Range check BEFORE computation. At raw_adc = 4095 the term
       (1.0f - v_ratio) is 0, the division gives +inf and the result is exactly
       -273.15 C; at raw_adc = 0, logf(0) = -inf gives the same number. An open
       or shorted thermistor would thus silently become "absolute zero".
       Passed to wm_to_cb() as the soil temperature (clamped there to 0 C), it
       would distort the temperature compensation and report the soil wetter
       than it is - for irrigation control a dangerous false result. */
    if (raw_adc < NTC_RAW_MIN || raw_adc > NTC_RAW_MAX)
    {
        return NTC_FALLBACK_C; // Open or shorted thermistor
    }

    float v_ratio = (float)raw_adc / ADC_MAX; // V_adc / VCC

    float r_ntc = R_FIXED * (v_ratio / (1.0f - v_ratio)); // NTC resistance from the divider ratio, Ohm

    float steinhart = logf(r_ntc / R_NOMINAL) / B_COEFF; // ln(R/R0) / B
    steinhart += 1.0f / T_NOMINAL;                       // + 1/T0 -> 1/T

    /* Guard against a division by zero: ln(R/R0)/B + 1/T0 could in theory be 0.
       This is far outside the operating range, but the check is cheap. */
    if (steinhart > -1e-9f && steinhart < 1e-9f)
    {
        return NTC_FALLBACK_C;
    }

    steinhart = 1.0f / steinhart; // Temperature in Kelvin

    return steinhart - 273.15f; // C°
}


/* One conversion -> degC, with a fallback on ADC failure. */
float temp_c_ntc(uint8_t chanel_adc){
    uint16_t raw = adc1_read(chanel_adc); // One conversion

    /* On a conversion timeout raw is 0. The range check in ntc_calc_temperature()
       would also reject it, but the error flag is checked explicitly so the
       result does not depend on what value a failed conversion returns. */
    if (adc1_last_error)
    {
        return NTC_FALLBACK_C; // ADC did not respond
    }

    return ntc_calc_temperature(raw);
}