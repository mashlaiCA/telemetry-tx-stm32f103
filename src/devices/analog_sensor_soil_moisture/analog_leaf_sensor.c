#include "analog_leaf_sensor.h"
#include "analog_sensors_manager.h"
#include "middleware/resistive_probe/resistive_probe.h"
#include "drivers/acd1/acd1.h" // adc1_last_error: failed samples are discarded

#define LEAF_PIN_A        3    // PA3: probe drive A
#define LEAF_PIN_B        4    // PA4: probe drive B
#define LEAF_ADC_CH       2    // ADC1 channel 2 = PA2: divider midpoint
#define LEAF_SET_US       300  // Settling time after each excitation, us

#define LEAF_SAMPLES      8    // Samples averaged per reading
#define LEAF_TH_WET     3000   // raw < 3000 -> WET
#define LEAF_TH_DRY     3800   // raw > 3800 -> DRY (gap 3000..3800 = hysteresis)
#define LEAF_RAW_MIN       5   // raw < 5 -> FAULT
#define LEAF_RAW_MAX    4095   // Full scale (not used in the code)

 uint16_t wetness_data; // Last averaged raw value (global, also returned by leaf_wetness_read())


static const resistive_probe_t probe = {
    LEAF_PIN_A, LEAF_PIN_B, LEAF_ADC_CH, LEAF_SET_US
};

static leaf_data_t data = { 0, LEAF_DRY }; // Last raw value and classified state

/* Configures the probe and resets the state. */
void leaf_wetness_init(void)
{
    resistive_probe_init(&probe); // Drive pins as outputs, sense pin analog, probe released
    data.raw = 0;
    data.state = LEAF_DRY;

}

/* Averages LEAF_SAMPLES probe readings, skipping failed ones. */
static uint16_t read_averaged(void)
{
    uint32_t sum = 0;  // Sum of successful samples
    uint8_t  taken = 0; // Number of successful samples

    for (uint8_t i = 0; i < LEAF_SAMPLES; i++)
    {
        uint16_t v = resistive_probe_read(&probe); // One forward + reverse measurement

        if (adc1_last_error)
        {
            continue; // Excitation already removed inside resistive_probe_read()
        }

        sum += v;
        taken++;
    }

    if (taken == 0u)
    {
        return 0; // Every sample failed (classified as FAULT by the caller)
    }

    return (uint16_t)(sum / taken); // Average of the successful samples
}

/* Measures, classifies with hysteresis and returns the raw average.
   The classified state is kept in this module only; callers receive the raw value. */
uint16_t leaf_wetness_read(void)
{
    wetness_data = read_averaged();
    data.raw = wetness_data;

    if (data.raw < LEAF_RAW_MIN) {
        data.state = LEAF_FAULT; // Implausibly low: short circuit or failed read
        return wetness_data;
    }


    if (data.state != LEAF_WET && data.raw < LEAF_TH_WET)
        data.state = LEAF_WET; // Became wet
    else if (data.state == LEAF_WET && data.raw > LEAF_TH_DRY)
        data.state = LEAF_DRY; // Dried out (only above the upper threshold: hysteresis)

    return wetness_data;
}



