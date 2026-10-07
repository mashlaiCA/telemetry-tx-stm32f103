#include "sht35_status.h"


/* Getter for the last SHT35 processing status. */
SHT35_Status_t get_last_sht35_error(void)
{
    return sht35_status; // Updated by SHT35_CRC_Check() and SHT35_Calculate()
}