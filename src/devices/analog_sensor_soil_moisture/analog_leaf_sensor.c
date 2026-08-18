#include "analog_leaf_sensor.h"
#include "analog_sensors_manager.h"
#include "middleware/resistive_probe/resistive_probe.h"

#define LEAF_PIN_A        3
#define LEAF_PIN_B        4
#define LEAF_ADC_CH       2
#define LEAF_SET_US       300

#define LEAF_SAMPLES      8
#define LEAF_TH_WET     3000      
#define LEAF_TH_DRY     3800      
#define LEAF_RAW_MIN       5    
#define LEAF_RAW_MAX    4095


static const resistive_probe_t probe = {
    LEAF_PIN_A, LEAF_PIN_B, LEAF_ADC_CH, LEAF_SET_US
};

static leaf_data_t data = { 0, LEAF_DRY, 0 };

void leaf_wetness_init(void)
{
    resistive_probe_init(&probe);
    data.raw = 0;
    data.state = LEAF_DRY;
   
}

static uint16_t read_averaged(void)
{
    uint32_t sum = 0;

    for (uint8_t i = 0; i < LEAF_SAMPLES; i++)
        sum += resistive_probe_read(&probe);

    return (uint16_t)(sum / LEAF_SAMPLES);
}

leaf_data_t leaf_wetness_read(void)
{
    data.raw = read_averaged();

    if (data.raw < LEAF_RAW_MIN || data.raw > LEAF_RAW_MAX) {
        data.state = LEAF_FAULT;
        return data;
    }


    if (data.state != LEAF_WET && data.raw < LEAF_TH_WET)
        data.state = LEAF_WET;
    else if (data.state == LEAF_WET && data.raw > LEAF_TH_DRY)
        data.state = LEAF_DRY;

    return data;
}

/*static int8_t leaf_sensor_id = -1;

void leaf_sensor_init(void)
{
   // analog_sensors_init();

    leaf_sensor_id =
        analog_sensor_add(2, 4090, 1); 
}

int16_t leaf_sensor_read_average(void)
{
    return
        analog_sensor_read_average(
            leaf_sensor_id
        );
}
        */

