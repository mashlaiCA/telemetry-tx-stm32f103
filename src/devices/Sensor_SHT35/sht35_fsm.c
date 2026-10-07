#include "sht35_fsm.h"
#include "application/errors/system_error.h"
#include "sensor_sht35.h"
#include "drivers/timeout_hw/timeout_hw.h"
#include "application/system_data/system_data.h"
#include "devices/analog_sensor_soil_moisture/analog_sensor_soil_moisture.h"

#include "application/errors/i2c_errors.h"



/* Period of recovery attempts while the sensor is disabled.
   One soft-reset attempt per 5 minutes is rare enough not to disturb DS3231
   on the shared I2C bus, yet lets a sensor that suffered a temporary fault
   come back without an MCU reset. */
#define SHT35_RETRY_PERIOD_MS   300000u   /* 5 min = 300 000 ms */

uint8_t sht_max_retry = 3;            // Consecutive errors allowed before the sensor is disabled
static uint8_t error_retry_count = 0; // Consecutive errors since the last successful measurement
timeout_t sht35_timeout;              // Not used in this module (the FSM uses sht35_context.timer)
uint8_t timeout_ms;                   // Not used in this module

SHT35_State_t sht35_state = st_idle; // Exported in sht35_fsm.h; the FSM itself uses sht35_context.state

/* State handler table, indexed by SHT35_State_t.
   Designated initializers bind every handler to its enum value explicitly,
   so reordering the enum cannot shift the table. A shifted table would call the
   wrong handler (e.g. st_error running State_Restart_Sensor, so failures are never
   counted) or leave an entry NULL (call through a NULL pointer -> HardFault). */
StateFunction_t state_table[st_count] = {
    [st_idle] = State_Idle,                                   // Wait until the last result is consumed
    [st_start_sensor] = State_Start_Sensor,                   // Send the measurement command
    [st_wait_measurement] = State_Wait_Measurement,           // Wait 20 ms for the conversion
    [st_read_data_measurement] = State_Read_Data_Measurement, // Read 6 raw bytes
    [st_crc_check] = State_CRC_Check,                         // Verify both CRC bytes
    [st_calculate_data] = State_Calculate_Data,               // Convert and publish T/RH
    [st_error] = State_Error,                                 // Count the failure, restart or disable
    [st_disable] = State_Disabled,                            // Sensor disabled, periodic recovery attempt
    [st_restart_sensor] = State_Restart_Sensor,               // Soft reset (+ bus recovery on failure)
};

/** @brief Runtime context of the SHT35 FSM. */
typedef struct
{
    SHT35_State_t state; // Current state
    // prev has the same type as state so the change detection in SHT35_FSM_Run()
    // compares values of one enum (avoids -Wenum-compare).
    SHT35_State_t prev;  // State during the previous SHT35_FSM_Run() call, for detecting state changes

    timeout_t timer; // Per-state timer armed in SHT35_OnEnter()
} sht35_cxt_t;       // SHT35 FSM context

sht35_cxt_t sht35_context = {
    .state = st_idle, // FSM starts in idle
    .prev = st_idle,  // Equal to state, so SHT35_OnEnter() is not called for the initial idle state
    .timer = {0}      // Timer not armed
};

/* Executes one FSM step: entry actions on a state change, then the handler of the
   current state. Never blocks; called from the main loop. */
void SHT35_FSM_Run(void)
{
    if (sht35_context.state != sht35_context.prev) // State changed since the last call
    {
        SHT35_OnEnter(sht35_context.state);       // Run entry actions once for the new state
        sht35_context.prev = sht35_context.state; // Remember it so the entry actions are not repeated
    }
    /* Guard against an out-of-range state or a missing handler: calling through
       a NULL or out-of-bounds table entry would HardFault. Disabling the sensor
       keeps the rest of the system running. */
    if (sht35_context.state >= st_count || state_table[sht35_context.state] == 0)
    {
        sht35_context.state = st_disable; // Fall back to the safe disabled state
        return;
    }

    sht35_context.state = state_table[sht35_context.state](); // Run the current state's handler and store the next state
}

/* Entry actions: arms the FSM timer for the states that measure time. */
void SHT35_OnEnter(SHT35_State_t st)
{
    switch (st) // Select the entry action for the new state
    {
    case st_idle:
        timeout_start(&sht35_context.timer, 50); // 50 ms idle delay (State_Idle() currently does not check it)
        break;

    case st_wait_measurement:
        timeout_start(&sht35_context.timer, 20); // 20 ms: covers the high-repeatability conversion time (max 15 ms per SHT3x datasheet)
        break;

    case st_disable:
        // Arm the recovery timer: next soft-reset attempt in SHT35_RETRY_PERIOD_MS
        timeout_start(&sht35_context.timer, SHT35_RETRY_PERIOD_MS);
        break;

    default: // Other states have no entry action
        break;
    }
}

/* Starts a new measurement as soon as system_data has consumed the previous
   result (DATA_SHT35_READY cleared). The 50 ms idle delay is disabled. */
SHT35_State_t State_Idle(void)
{
   // if (!timeout_has_expired(&sht35_context.timer)) // Wait for 50 ms before starting the next measurement cycle to ensure the sensor is ready
       // return st_idle; // If the delay fails, transition to the idle state


   if (!(system_data.ready_sensors_flag & DATA_SHT35_READY)) //!!!!!!!@@@@
    {
        return st_start_sensor; // Previous result consumed: start a new measurement
    }
    return st_idle; //!!!!!!
}

/* Sends the measurement command to SHT35.
   The result of this transaction is checked directly instead of the global
   I2C status: the bus is shared with DS3231, so the global value may hold
   an error from another device. */
SHT35_State_t State_Start_Sensor(void)
{
    if (I2C_Write_Sensor_SHT35() != i2c_ok) // Command not accepted (bus busy / NACK / timeout)
    {
        return st_error; // Go to error handling (retry / restart)
    }
    return st_wait_measurement; // Command accepted: wait for the conversion
}

/* Waits for the 20 ms conversion timer armed in SHT35_OnEnter(). */
SHT35_State_t State_Wait_Measurement(void)
{

    if (!timeout_has_expired(&sht35_context.timer)) // 20 ms conversion time not elapsed yet
    {
        return st_wait_measurement; // Keep waiting
    }
    return st_read_data_measurement; // Conversion finished: read the result
}

/* Reads the 6-byte result. As in State_Start_Sensor(), the status of this
   transaction is checked, not the shared global I2C status. */
SHT35_State_t State_Read_Data_Measurement(void)
{
    if (I2C_Read_Sensor_SHT35() != i2c_ok) // Read failed (bus busy / NACK / timeout)
    {
        return st_error; // Go to error handling
    }
    return st_crc_check; // Data received: verify the CRC
}

/* Checks both CRC bytes; the result is taken from the driver's global status,
   which SHT35_CRC_Check() updates on both success and failure. */
SHT35_State_t State_CRC_Check(void)
{
    SHT35_CRC_Check(); // Verify CRC-8 of T and RH words; on success extract raw T/RH

    if (get_last_sht35_error() != sht35_ok) // CRC mismatch on either word
    {
        return st_error; // Corrupted data: go to error handling
    }
    return st_calculate_data; // CRC OK: convert the raw values
}
/* Converts raw data to degC / %RH, publishes the result and resets the
   consecutive-error counter. */
SHT35_State_t State_Calculate_Data(void)
{
    SHT35_Calculate(); // Convert raw words and range-check the results

    if (get_last_sht35_error() != sht35_ok) // Temperature or humidity out of range
    {
        return st_error; // Implausible value: go to error handling
    }


    system_data.ready_sensors_flag |= DATA_SHT35_READY;//!@!!!!!!!@@@@

    /* Reset after every successful measurement, so only CONSECUTIVE failures
       count toward sht_max_retry. Otherwise rare isolated glitches (EMI,
       condensation, supply dips) accumulated over weeks would disable a
       sensor that actually works, and the node would keep sending packets
       with stale air temperature/humidity. */
    error_retry_count = 0;

    return st_idle; // Measurement complete: wait for the next request
}

/* Sends a soft reset. If the sensor does not acknowledge it, a slave may be
   holding SDA low after an interrupted transaction, so the bus is recovered
   before the next attempt. */
SHT35_State_t State_Restart_Sensor(void)
{
    if (I2C_Restart_Sensor_SHT35() != i2c_ok) // Soft reset not acknowledged
    {
        (void)i2c_bus_recover(); // Clock out a stuck slave and re-init I2C1
        return st_error;         // Count this as another consecutive error
    }

    // SHT35 needs up to 1.5 ms after a soft reset before it accepts commands.
    // The 50 ms st_idle timer (SHT35_OnEnter) is meant to cover this, but
    // State_Idle() does not check it, so the next command is not guaranteed to wait;
    // a NACK in that case is counted as another consecutive error.
    return st_idle; // Reset accepted: resume the normal measurement cycle
}

/* Counts consecutive failures: restart while below sht_max_retry,
   otherwise report a system error and disable the sensor. */
SHT35_State_t State_Error(void)
{
    error_retry_count++;                   // One more consecutive failure
    if (error_retry_count < sht_max_retry) // Limit not reached yet
    {
        return st_restart_sensor; // Try a soft reset
    }

    set_system_error(system_error_sht35); // Record the fault (only if no other system error is recorded yet)
    error_retry_count = 0;                // Start counting from zero after the next recovery attempt

    get_system_error(); // Debug aid: the return value is discarded (handy as a breakpoint location)

    return st_disable; // Stop touching the bus; recovery is retried every 5 min
}

/* Sensor considered faulty: the shared I2C bus is left alone (so DS3231 is
   not disturbed) and DATA_SHT35_READY is forced, so system_data_run() does not
   wait forever and builds the packet with the last valid values. */
SHT35_State_t State_Disabled(void)
{
    system_data.ready_sensors_flag |= DATA_SHT35_READY; // Do not block packet creation

    /* The disabled state is reversible: a sensor disabled by a temporary bus
       fault would otherwise stay off until an MCU reset, which nobody can do in
       the field. One attempt per SHT35_RETRY_PERIOD_MS (5 min) adds only one
       extra I2C exchange per 5 minutes on the bus shared with DS3231. */
    if (!timeout_has_expired(&sht35_context.timer)) // Retry period still running
    {
        return st_disable; // Stay disabled
    }

    timeout_start(&sht35_context.timer, SHT35_RETRY_PERIOD_MS); // Re-arm for the next attempt

    error_retry_count = 0; // Give the recovered sensor the full sht_max_retry budget

    return st_restart_sensor; // Soft reset and return to the working cycle
}
