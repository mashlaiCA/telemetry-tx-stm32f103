
#include "stm32f1xx.h"
#include "board/board_pins.h"

#include "application/calibration_analog_signal/calibration_analog_signal.h"

#include "devices/analog_sensor_soil_moisture/analog_sensors_manager.h"
#include "devices/analog_sensor_soil_moisture/analog_sensor_soil_moisture.h"
#include "devices/analog_sensor_soil_moisture/analog_leaf_sensor.h"

#include "radiolib_stm32_hal/radiolib_stm32_hal.h"
#include "protocols/spi/spi.h"
#include "devices/lora_sx1276/lora_sx1276.h"
#include "protocols/uart/uart.h"
#include "application/time/time.h"
#include "devices/lora_sx1276/lora_fsm.h"
#include "protocols/i2c/i2c.h"
#include "devices/Sensor_SHT35/sensor_sht35.h"
#include "devices/Sensor_SHT35/sht35_fsm.h"
#include "application/system_data/system_data.h"
#include "devices/analog_sensor_soil_moisture/analog_sensor_soil_moisture_fsm.h"
#include "drivers/external_interrupt/exti_1.h"

#include "devices/ds3231_rtc/ds3231_rtc.h"
#include "devices/watermark_200ss/watermark_200ss.h"
#include "devices/watermark_200ss/watermark_fsm.h"

#include "drivers/tim/tim2_hw.h"// Not referenced directly in this file
#include "drivers/gpio/gpio_hw.h"// Not referenced directly in this file

#include "drivers/power/sleep_hw.h" // sleep_enter_stop_for(), sleep_debug_enable()
#include "drivers/exti/exti_hw.h"   // EXTI0 wakeup line, DIO0 mask/unmask
#include "drivers/acd1/acd1.h"

#include "devices/ntc/ntc.h"
#include "drivers/watchdog/iwdg_hw.h"


#define CYCLE_PERIOD_S       30u                        // Wakeup-to-wakeup period of the measurement cycle, s
#define CYCLE_PERIOD_MS      (CYCLE_PERIOD_S * 1000u)   // Same period in ms

#define MEASURE_WINDOW_MS    10000u // Maximum active phase: sleep anyway if the packet is not sent by then

#define SLEEP_MIN_S          2u  // Shortest sleep: ds3231_set_alarm_in() needs at least 2 s ahead
#define SLEEP_CHUNK_MAX_S    12u // Longest single Stop interval, limited by the IWDG timeout


// Active part (measure + send) must leave time for sleep within one cycle
static_assert(MEASURE_WINDOW_MS < CYCLE_PERIOD_MS,
              "MEASURE_WINDOW_MS must be shorter than the cycle period");

// LoRa TX/join must finish before the MCU goes back to sleep
static_assert(LORA_TX_JOIN_MS < MEASURE_WINDOW_MS,
              "LORA_TX_JOIN_MS must fit inside the measurement window");

// IWDG keeps counting in Stop mode; worst-case LSI (60 kHz) gives ~17.5 s,
// so each sleep chunk must end early enough to refresh the watchdog
static_assert(SLEEP_CHUNK_MAX_S <= 15u,
              "Sleep chunk must stay well below the IWDG limit (~17.5 s)");


extern STM32F103RadioLibHal hal; // RadioLib HAL instance (radiolib_stm32_hal.cpp)

Module module(&hal, 0, 1, 2, RADIOLIB_NC); // HAL pin numbers: NSS = 0, DIO0 (IRQ) = 1, RST = 2, no GPIO/BUSY pin
SX1276 radio(&module);                     // RadioLib driver used by the LoRa FSM

timeout_t system_timeout; // Active-phase window (MEASURE_WINDOW_MS) of the current cycle

timeout_t boot_delay_timer; // 200 ms startup delay

uint16_t count = 1; // !

char line[26]; // Output of ds3231_datetime_to_str() (25 characters + '\0')


static ds3231_time_t g_cycle_wake_time; // RTC time of the current cycle's wakeup
static uint8_t       g_cycle_wake_valid = 0; // 1 if g_cycle_wake_time was read successfully

/* Seconds since start of day - convenient for arithmetic across midnight. */
static uint32_t tod_from_time(const ds3231_time_t *t)
{
    return (uint32_t)t->hour * 3600u + (uint32_t)t->min * 60u + (uint32_t)t->sec;
}

/* Records the RTC time of the wakeup; the next alarm target is computed from it. */
static void cycle_mark_wake(void)
{
    ds3231_time_t now;

    if (ds3231_get_time(&now) == i2c_ok)
    {
        g_cycle_wake_time = now;
        g_cycle_wake_valid = 1;
    }
    else
    {
        g_cycle_wake_valid = 0; // Without RTC the exact target can't be computed - fallback path
    }
}


/* Puts everything that would draw current or wake the MCU into a safe state before Stop mode. */
static void system_power_down_peripherals(void)
{
    uart_flush(); // Wait until the last character leaves the shift register

    lora_radio_sleep(); // SX1276: STANDBY (~1.5 mA) -> SLEEP (~0.2 uA)
    adc1_sleep();       // ADC1 with ADON=1 draws ~1 mA - more than the whole MCU in STOP

    exti1_dio0_mask(); // DIO0 must not wake the MCU from sleep
}


/* One Stop-mode interval until the DS3231 alarm.
   Returns 1 after a sleep, 0 if the alarm line is already low (no sleep). */
static uint8_t system_stop_once(uint32_t expected_ms)
{
    /* Stale PR flags (lines 0 and 1) and NVIC pending bits would make __WFI()
       return immediately, so they are cleared first. */
    exti_clear_pending_wakeup_lines();


    if (exti0_pa0_level() == 0)
    {
        return 0; // Alarm already fired - must not sleep
    }

    exti0_wakeup_flag = 0; // Set again by EXTI0_IRQHandler on the alarm

    /* IWDG counts in STOP too, so the counter is reloaded right before
       sleeping: a 12 s chunk vs a ~17.5 s ceiling in the worst case. */
    iwdg_kick();

    sleep_enter_stop_for(expected_ms); // <- STOP; returns only after wakeup

    iwdg_kick(); // Fresh watchdog period for the active phase

    return 1;
}

/* Restores peripherals after wakeup. */
static void system_restore_after_stop(void)
{
    exti1_dio0_unmask(); // Unmasks and also clears any edge that arrived while masked

    adc1_wake();      // ADON + recalibration
    lora_radio_wake(); // SLEEP -> STANDBY, LoRa configuration is retained

    /* A1F holds INT/SQW at "0". Until it is cleared, the next alarm will not
       produce a falling edge on PA0 - i.e. there will be no second wakeup. */
    if (ds3231_clear_alarm_flag() != i2c_ok)
    {
        (void)i2c_recover(); // An interrupted transfer may have left the bus busy
        (void)ds3231_clear_alarm_flag(); // One retry after recovery; result not checked
    }
}

/* Seconds left until the end of the current cycle (wake time + CYCLE_PERIOD_S).
   Returns 0 if the wake time or the current time is unknown, or the period is used up. */
static uint32_t cycle_remaining_s(void)
{
    ds3231_time_t now;

    if (!g_cycle_wake_valid || ds3231_get_time(&now) != i2c_ok)
    {
        return 0; // Cannot compute: caller uses the fallback
    }

    uint32_t wake_tod = tod_from_time(&g_cycle_wake_time);
    uint32_t now_tod = tod_from_time(&now);

    /* Difference modulo one day - handles midnight correctly. */
    uint32_t elapsed = (now_tod + 86400u - wake_tod) % 86400u;

    if (elapsed >= CYCLE_PERIOD_S)
    {
        return 0; // Active phase ate the whole period
    }

    return CYCLE_PERIOD_S - elapsed;
}

/* Sleeps until the end of the cycle (wake time + CYCLE_PERIOD_S), in chunks of
   at most SLEEP_CHUNK_MAX_S so the watchdog can be reloaded between them, then
   restores the peripherals and triggers the next measurement. */
static void system_sleep_cycle(void)
{
    /* 1. Do not sleep while LoRa is transmitting: in STOP the SPI clock and
          DIO0 line stop, the packet is cut off and lora_busy gets stuck at 1.
          The caller then simply starts a new active window without sleeping. */
    if (!lora_fsm_is_idle())
    {
        return;
    }

    uint32_t target_tod = 0;  // Time of day of the next wakeup, s
    uint8_t  target_valid = 0; // 1 if target_tod is known

    if (g_cycle_wake_valid)
    {
        target_tod = (tod_from_time(&g_cycle_wake_time) + CYCLE_PERIOD_S) % 86400u; // Wake time + period, wrapped at midnight
        target_valid = 1;
    }

    system_power_down_peripherals();

    for (;;)
    {
        uint32_t remaining = cycle_remaining_s(); // Seconds to the target, 0 if unknown

        if (remaining < SLEEP_MIN_S)
        {
            /* Too little time left to arm an alarm, or the time is unknown.
               With a known target the cycle has overrun: no sleep, start the
               next cycle now. Without a target (RTC unreadable) sleep the
               minimum time, so the loop does not run back to back. */
            if (!target_valid)
            {
                if (ds3231_set_alarm_in(SLEEP_MIN_S) == i2c_ok)
                {
                    (void)system_stop_once(SLEEP_MIN_S * 1000u);
                }
                else
                {
                    uart_send_string("alarm err");
                    (void)i2c_recover(); // Alarm not armed: try to release the bus
                }
            }
            break;
        }

        if (remaining > SLEEP_CHUNK_MAX_S)
        {
            /* Intermediate chunk: accuracy is not important here, only staying
               below the watchdog ceiling. The ±1 s error of this chunk does not
               accumulate because the last chunk targets an ABSOLUTE time. */
            if (ds3231_set_alarm_in((uint8_t)SLEEP_CHUNK_MAX_S) != i2c_ok)
            {
                uart_send_string("alarm err");
                (void)i2c_recover();
                break; // Give up sleeping in this cycle
            }

            if (!system_stop_once(SLEEP_CHUNK_MAX_S * 1000u))
            {
                break; // Alarm fired during preparation - return to the loop
            }

            continue; // Recompute the remaining time and sleep again
        }

        /* Last chunk - lands exactly on the target. It defines the period
           accuracy: the alarm fires on the same second boundary we counted
           from, so period = exactly CYCLE_PERIOD_S. */
        uint8_t th = (uint8_t)(target_tod / 3600u);          // Target hours
        uint8_t tm = (uint8_t)((target_tod % 3600u) / 60u);  // Target minutes
        uint8_t ts = (uint8_t)(target_tod % 60u);            // Target seconds

        if (ds3231_set_alarm_at(th, tm, ts) != i2c_ok)
        {
            uart_send_string("alarm err");
            (void)i2c_recover();
            break;
        }

        (void)system_stop_once(remaining * 1000u);
        break;
    }

    cycle_mark_wake(); // This wakeup is the reference for the next cycle

    system_restore_after_stop();

    watermark_fsm_start_now(); // Trigger the Watermark measurement for the new cycle
}

/* Startup, then the endless measure -> transmit -> sleep cycle. */
int main(void) // Main function
{

  uint8_t recovered_from_hang = iwdg_reset_occurred(); // Read (and clear) the reset cause before anything else

  iwdg_debug_freeze(); // Development: don't reset the MCU at breakpoints
  iwdg_start();        // Started first, so a hang anywhere during initialization is also covered

  pins_init();    // GPIO clocks, SPI1 pins, SX1276 RST/DIO0
  EXTI1_init();   // Initialize external interrupt on PB1 for DIO0
  __enable_irq(); // Enable global interrupts
  spi_start();    // Initialize SPI peripheral
  lora_init();    // SX1276 control pins (RST, DIO0)

  timer_start(); // Start the system timer for timekeeping

  // The watchdog is already running (started at the top of main()).

  i2c_SDA_SCL(6); // SCL
  i2c_SDA_SCL(7); // SDA
  i2c_start();    // Initialize I2C peripheral

  hal.spiBegin(); // RadioLib HAL SPI init (calls spi_start() again)

  iwdg_kick();

  timer_set(&boot_delay_timer, 200); // 200 ms startup delay for the external devices
  while (!timer_wait(&boot_delay_timer))
    ;

  iwdg_kick();

  analog_sensors_init(); // ADC1 init + calibration
  leaf_wetness_init();   // Leaf probe pins
  soil_sensor_init();    // Analog soil sensor registration (ADC channel 8)
  ntc_init();            // PA1 analog input

  lora_fsm_init(&radio);    // LoRa FSM; radio.begin() runs on the first lora_fsm_run()
  analog_sensor_fsm_init(); // DATA_ANALOG_READY publisher
  uart_init();              // Debug UART (9600 8N1)

  watermark_fsm_init();//200ss - calls watermark_init()

  iwdg_kick(); // before I2C work - the longest part of initialization

 //probe_pin_init();//test timing

  //=========ds3231_RTC==================

  ds3231_init(); // INT/SQW in alarm mode, alarm flags cleared (result not checked)

  sleep_debug_enable();   // Development only: SWD stays alive in STOP (increases Stop current)
  exti0_pa0_init();       // Required: PA0/EXTI0 is the only wakeup source (DS3231 alarm)

  if (ds3231_lost_power()) // OSF set or RTC unreadable
  {
    ds3231_time_t t = {
        .sec = 0, .min = 17, .hour = 10, .day = 6, .date = 12, .month = 9, .year = 26}; // Placeholder: 12-09-2026 10:17:00
    ds3231_set_time(&t);
  }

  ds3231_clear_alarm_flag(); // Release INT/SQW in case an alarm fired before the reset


  cycle_mark_wake(); // Startup counts as the first wakeup: reference for the cycle period


  watermark_fsm_start_now(); // First Watermark measurement right away

  iwdg_kick();


  if (recovered_from_hang)
  {
    uart_send_string("IWDG RESET"); // Report that the previous run ended in a watchdog reset
  }


  timer_set(&system_timeout, MEASURE_WINDOW_MS); // Active-phase window of the first cycle

  uint32_t cycle_start_tx = lora_tx_packet_count; // packet count at the start of the cycle

  while (1)
  {

    iwdg_kick();

    /* --- Sensor FSMs: SHT35, analog (leaf) flag, Watermark (incl. soil NTC) --- */
    SHT35_FSM_Run();
    analog_sensor_FSM_Run();
    watermark_fsm_run();

    /* --- LoRa transmission --- */
    lora_fsm_run();

    system_data_run(); // Build the payload once all sources are ready

    /* The active phase ends either when the packet has ACTUALLY been sent
       (lora_tx_packet_count changed) or when the MEASURE_WINDOW_MS window
       expires. The window makes the node sleep anyway, so one hung sensor
       cannot keep the board powered; ending on data_sent lets the node sleep
       as soon as its work is done instead of on a fixed timer. */
    uint8_t data_sent = (lora_tx_packet_count != cycle_start_tx);
    uint8_t window_expired = timer_wait(&system_timeout);

    if (!data_sent && !window_expired)
    {
      continue; // Keep running the FSMs
    }

    // =============================
    ds3231_time_t now;

    if (ds3231_get_time(&now) == i2c_ok)
    {
      /* Watermark data is measured by watermark_fsm_run() and sent by the
         LoRa FSM; only the timestamp is echoed here for debugging. */
      ds3231_datetime_to_str(&now, line);
      uart_send_string(line);
    }
    else
    {
      uart_send_string("error");

      (void)i2c_recover(); // RTC read failed: try to release the bus
    }
    //=========================


    lora_debug_dump(); // One-line LoRa state (before the radio is put to SLEEP)

    {
      char wm_line[WM_PACKET_LEN + 4]; // "WM " + Watermark debug text + '\0'
      char *p = pack_str(wm_line, "WM ");
      p = pack_str(p, watermark_fsm_ready() ? watermark_fsm_packet() : "none");
      *p = '\0';
      uart_send_string(wm_line);
    }


    system_sleep_cycle(); // Sleep until the end of the cycle (returns at once if LoRa is busy)

    if (exti0_wakeup_flag)
    {
      uart_send_string("wake\r\n"); // first line after wake
      exti0_wakeup_flag = 0;
    }

    cycle_start_tx = lora_tx_packet_count;         // Reference for the next cycle's data_sent
    timer_set(&system_timeout, MEASURE_WINDOW_MS); // New active-phase window
  }
}
