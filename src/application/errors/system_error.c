#include "system_error.h"

System_Error_t system_error_flags = system_error_none; // No error recorded at startup

/* Latches the first error; later errors are ignored. */
void set_system_error(System_Error_t error)
{
    if (system_error_flags == system_error_none) // Nothing recorded yet
    {
        system_error_flags = error; // Keep this one as the root cause
    }
}

/* Getter for the latched error. */
System_Error_t get_system_error(void)
{
    return system_error_flags; // First recorded error or system_error_none
}
