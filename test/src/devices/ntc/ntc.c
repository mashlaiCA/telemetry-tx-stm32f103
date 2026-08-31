#include "ntc.h"
#include "math.h"
#include "drivers/gpio/gpio_hw.h"

#define R_FIXED 10000.0f
#define R_NOMINAL 10000.0f
#define T_NOMINAL 298.15f
#define B_COEFF 3950.0f
#define ADC_MAX 4095.0f

void ntc_init(void)
{
    ntc_gpio_init();
}

static float ntc_calc_temperature(uint16_t raw_adc)
{
    float v_ratio = (float)raw_adc / ADC_MAX;

    float r_ntc = R_FIXED * (v_ratio / (1.0f - v_ratio));

    float steinhart = logf(r_ntc / R_NOMINAL) / B_COEFF;
    steinhart += 1.0f / T_NOMINAL;
    steinhart = 1.0f / steinhart;

    return steinhart - 273.15f; // C°
}


float temp_c_ntc(uint8_t chanel_adc){
    uint16_t raw = adc1_read(chanel_adc);
      return ntc_calc_temperature(raw);
}