#ifndef SHT35_STATUS_H
#define SHT35_STATUS_H


typedef enum {
    sht35_ok = 0, // No error
    sht35_error_crc, // CRC check failed
    sht35_calc_out_of_range, // Calculated value is out of valid range
    sht35_error_calculation, // General calculation error

    sht35_count // Total number of status codes
} SHT35_Status_t; // Enumeration for SHT35 error status

extern SHT35_Status_t sht35_status;  // Global variable to hold the last SHT35 error status

/**
 * @brief Get the last SHT35 error status.
 * This function returns the current value of the global error status variable, which holds
 */
SHT35_Status_t get_last_sht35_error(void); 

#endif 