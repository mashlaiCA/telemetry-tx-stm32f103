#include "system_error.h"

System_Error_t system_error_flags = system_error_none; // Initialize system error flags to none

void set_system_error(System_Error_t error)// Function to set the system error flags
{
    if(system_error_flags == system_error_none) // Only set the system error if there are no existing errors to avoid overwriting previous error states
    {
        system_error_flags = error; // Set the system error flags to the specified error
}
}

System_Error_t get_system_error(void)
{
    return system_error_flags; // Return the current system error flags
}