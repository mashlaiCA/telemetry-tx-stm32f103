#include "drivers/uart_hw/uart_hw.h"
#include "uart.h"

/* Initializes USART1 through the driver. */
void uart_init(void)
{
    uart_hw_init(); // Initialize hardware UART
}

/* Exposes uart_hw_flush() at the protocol level: the application must wait
   for transmission to finish before entering Stop mode. */
void uart_flush(void)
{
    uart_hw_flush(); // Wait for SR.TC
}

/* Sends a string followed by LF. */
void uart_send_string(const char *str)
{
    while (*str != '\0') // Loop until null terminator
    {
        uart_hw_send_byte((uint8_t)*str); // Send each character
        str++;                            // Move to next character
    }
    uart_hw_send_byte('\n'); // Line ending (LF only)
}

/* Sends a string followed by LF (from uart_send_string()) and CR LF. */
void uart_send_line(const char *str){
    uart_send_string(str);   // str + '\n'
    uart_hw_send_byte('\r'); // Additional CR
    uart_hw_send_byte('\n'); // Additional LF
}

/* Sends a signed integer as decimal text followed by CR LF. */
void uart_print_int(int value){
    char buf[16]; // Digits in reverse order
    int i = 0;    // Number of digits stored

    if(value == 0){
        uart_hw_send_byte('0'); // Special case: no digits would be produced by the loop
    }else{
        if(value < 0)
        {
            uart_hw_send_byte('-'); // Sign
            value = -value;         // Work with the magnitude
        }
        while(value > 0 && i < sizeof(buf)-1){
            buf[i++] = (value % 10) + '0'; // Least significant digit first
            value /= 10;
        }
    }
    while(i--){
        uart_hw_send_byte(buf[i]); // Send digits most significant first
    }
    uart_hw_send_byte('\r');
    uart_hw_send_byte('\n');
}

/* Sends a signed 32-bit integer as decimal text, no line ending. */
void uart_print_int_raw(int32_t value)
{
    char buf[12]; // Digits in reverse order (10 digits max for int32)
    int i = 0;    // Number of digits stored

    if (value < 0) { uart_hw_send_byte('-'); value = -value; } // Sign, then the magnitude

    if (value == 0) { uart_hw_send_byte('0'); return; } // Special case: zero

    while (value > 0 && i < (int)sizeof(buf) - 1) {
        buf[i++] = (value % 10) + '0'; // Least significant digit first
        value /= 10;
    }
    while (i--) uart_hw_send_byte(buf[i]); // Most significant digit first
}

/* Sends a float with one decimal digit, rounded half up. */
void uart_print_float1(float v)
{
    if (v < 0.0f) { uart_hw_send_byte('-'); v = -v; } // Sign, then the magnitude

    int32_t whole = (int32_t)v;                                 // Integer part
    int32_t frac  = (int32_t)((v - (float)whole) * 10.0f + 0.5f); // First decimal, rounded

    if (frac >= 10) { whole++; frac = 0; } // Rounding carried into the integer part (e.g. 1.96 -> 2.0)

    uart_print_int_raw(whole);
    uart_hw_send_byte('.');
    uart_hw_send_byte('0' + frac);
}

/* Sends an unsigned 16-bit value as decimal text, no line ending. */
void uart_send_uint16_t(uint16_t value)
{
    char buffer[6] = {0}; // Up to 5 digits (65535) in reverse order
    int index = 0;        // Number of digits stored

    if (value == 0) // Handle zero case
    {
        uart_hw_send_byte('0'); // Send '0' for zero value
        return;                 // Return early
    }
    while (value > 0 && index < sizeof(buffer) - 1) // Convert value to string in reverse order
    {
        buffer[index++] = (value % 10) + '0'; // Get least significant digit and convert to character
        value /= 10;                          // Remove least significant digit
    }
    while (index > 0) // Send the string in correct order
    {
        uart_hw_send_byte(buffer[--index]); // Send each character
    }
}

/* Sends four values on one line. Digits are stored reversed and the separator
   is appended after them, so on the wire the space comes FIRST: " v1 v2 v3 v4\n".
   A zero value produces only the space. */
void uart_send_uint16_t2(uint16_t value1,
                         uint16_t value2,
                         uint16_t value3,
                         uint16_t value4)
{
    uint16_t values[4] = {value1, value2, value3, value4}; // Values to send
    char buffer[32] = {0};                                 // Digits of one value (reversed) + separator
    uint8_t len = 4;                                       // Number of values to send

    for (uint8_t i = 0; i < len; i++) // Loop through each value
    {
        uint16_t value = values[i]; // Get current value
        uint8_t index = 0;          // Index for buffer

        while (value > 0 && index < sizeof(buffer) - 1) // Convert value to string in reverse order
        {
            buffer[index++] = (value % 10) + '0'; // Get least significant digit and convert to character
            value /= 10;                          // Remove least significant digit
        }
        buffer[index++] = ' '; // Separator; sent first because the buffer is sent backwards

        while (index > 0) // Send the buffer backwards: separator, then digits most significant first
        {
            uart_hw_send_byte(buffer[--index]); // Send each character
        }
    }

    uart_hw_send_byte('\n'); // Send newline at the end of the line
}
//=============================
/* Writes a signed 32-bit integer as decimal text at p (no '\0').
   Returns the position after the last character. */
char *pack_int(char *p, int32_t value)
{
    char buf[12]; // Digits in reverse order
    int i = 0;    // Number of digits stored

    if (value < 0) { *p++ = '-'; value = -value; } // Sign, then the magnitude

    if (value == 0) {
        *p++ = '0'; // Special case: zero
        return p;
    }

    while (value > 0 && i < (int)sizeof(buf)) {
        buf[i++] = (char)('0' + (value % 10)); // Least significant digit first
        value /= 10;
    }
    while (i--) *p++ = buf[i]; // Copy digits most significant first

    return p;
}

/* Writes a float with one decimal digit (rounded half up) at p (no '\0').
   Returns the position after the last character. */
char *pack_float1(char *p, float v)
{
    if (v < 0.0f) { *p++ = '-'; v = -v; } // Sign, then the magnitude

    int32_t whole = (int32_t)v;                                 // Integer part
    int32_t frac  = (int32_t)((v - (float)whole) * 10.0f + 0.5f); // First decimal, rounded
    if (frac >= 10) { whole++; frac = 0; } // Rounding carried into the integer part

    p = pack_int(p, whole);
    *p++ = '.';
    *p++ = (char)('0' + frac);

    return p;
}

/* Copies s to p without the terminator; returns the position after it. */
char *pack_str(char *p, const char *s)
{
    while (*s) *p++ = *s++; // Copy until '\0' (not copied)
    return p;
}

