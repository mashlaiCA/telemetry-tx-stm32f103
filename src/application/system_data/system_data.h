/**
 * @file  system_data.h
 * @brief Shared sensor data and assembly of the LoRa payload string.
 * This file provides:
 * 1. Readiness bits of the sensor FSMs (DATA_*_READY).
 * 2. system_data_t / system_data - latest values, payload string and handshake flags.
 * 3. system_data_run() - collects results and builds the payload once all sources are ready.
 * 4. Helpers: sensor_update_*(), data_creation(), int_to_str().
 * Handshake: sensor FSMs set DATA_*_READY -> system_data_run() builds data_string
 * and sets ready_data_creation_flag -> the LoRa FSM transmits it and clears the flags.
 */

#ifndef SYSTEM_DATA_H
#define SYSTEM_DATA_H

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif


#define DATA_SHT35_READY (1 << 1)  // SHT35 has a new temperature/humidity result (or is disabled)
#define DATA_ANALOG_READY (1 << 0) // Leaf sensor may be read (set by the analog FSM; soil sensor not involved)

/* Watermark ready bit.
   soil_moisture_10 comes from Watermark, and the Watermark measurement and the
   SHT35 measurement complete at about the same time after wakeup, in arbitrary
   order. Without a separate bit data_creation() could run before
   watermark_state_evaluate() and put the PREVIOUS cycle's cb into the packet
   (or 0 = "saturated" on the first cycle after power-on). Set by
   watermark_fsm.c after the measurement, cleared at the start of a new one. */
#define DATA_WATERMARK_READY (1 << 2)


/** @brief Latest sensor values, payload string and handshake flags. */
typedef struct
{
    uint16_t temperature; // Air temperature from SHT35, whole degC (negative clamped to 0)
    uint16_t humidity;    // Air relative humidity from SHT35, whole %RH
    uint16_t soil_moisture_10; // Soil water tension from Watermark, cb: 0..239, 240 = SHORT, 255 = OPEN
    uint16_t leaf_moisture;    // Leaf sensor averaged raw value, 0..4095 (lower = wetter)

    /* Payload string, 10 positional comma-separated fields, no labels:
         cb,humidity,temperature,leaf,DD,MM,YYYY,hh,mm,ss
       Worst case (int_to_str() prints uint16_t values with up to 5 characters):
       - RTC responded: "65535,65535,65535,65535" (23)
         + ",DD,MM,YYYY,hh,mm,ss" (3+3+5+3+3+3 = 20) + '\0' = 44 bytes;
       - RTC did not respond: 23 + ",E,E,E,E,E,E" (12) + '\0' = 36 bytes.
       pack_two_digits() always writes exactly 2 characters, so the length is
       bounded even with corrupted RTC registers. 48 bytes cover the worst case. */
    char data_string[48];

    volatile uint8_t ready_sensors_flag;       // DATA_*_READY bits set by the sensor FSMs
    volatile uint8_t ready_data_creation_flag; // 1 = data_string built and waiting for LoRa; cleared after TX done
    volatile uint8_t lora_busy;   // 1 while the LoRa FSM is preparing/transmitting a packet

} system_data_t;

extern system_data_t system_data; // Single shared instance

void system_data_run(void); // Collects sensor results and builds the payload when all are ready; call from the main loop

void sensor_update_SHT35(system_data_t* data); // Copies SHT35 temperature/humidity into data

void sensor_update_soil_moisture(system_data_t* data); // Copies the analog soil sensor value (not called in the current data path)

void sensor_update_leaf_sensor(system_data_t* data); // Measures the leaf sensor (blocking, 8 probe reads) into data->leaf_moisture

void data_creation(system_data_t* data); // Builds data_string from the values in data plus the DS3231 date/time

char* int_to_str(int value, char* str); // Writes value as decimal text at str (no '\0'); returns the position after it

#ifdef __cplusplus
}
#endif

#endif // SYSTEM_DATA_H
