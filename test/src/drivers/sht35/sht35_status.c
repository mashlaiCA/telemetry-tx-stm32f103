#include "sht35_status.h"


SHT35_Status_t get_last_sht35_error(void) // Function to retrieve the last SHT35 error status
{
    return sht35_status; // Return the current value of the global error status variable
}