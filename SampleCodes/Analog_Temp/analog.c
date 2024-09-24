// Analog Example
// Jason Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL with LCD/Temperature Sensor
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz
// Stack:           4096 bytes (needed for sprintf)

// Hardware configuration:
// LM60 Temperature Sensor:
//   AIN3/PE0 is driven by the sensor
//   (V = 424mV + 6.25mV / degC with +/-2degC uncalibrated error)
// UART Interface:
//   U0TX (PA1) and U0RX (PA0) are connected to the 2nd controller
//   The USB on the 2nd controller enumerates to an ICDI interface and a virtual COM port
//   Configured to 115,200 baud, 8N1

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "tm4c123gh6pm.h"
#include "wait.h"
#include "uart0.h"
#include "adc0.h"

// PortE masks
#define AIN3_MASK 1

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// Initialize Hardware
void initHw()
{
	// Configure HW to work with 16 MHz XTAL, PLL enabled, system clock of 40 MHz
    SYSCTL_RCC_R = SYSCTL_RCC_XTAL_16MHZ | SYSCTL_RCC_OSCSRC_MAIN | SYSCTL_RCC_USESYSDIV | (4 << SYSCTL_RCC_SYSDIV_S);

    // Set GPIO ports to use APB (not needed since default configuration -- for clarity)
    // Note UART on port A must use APB
    SYSCTL_GPIOHBCTL_R = 0;

    // Enable clocks
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R4;
    _delay_cycles(3);

    // Configure AIN3 as an analog input
	GPIO_PORTE_AFSEL_R |= AIN3_MASK;                 // select alternative functions for AN3 (PE0)
    GPIO_PORTE_DEN_R &= ~AIN3_MASK;                  // turn off digital operation on pin PE0
    GPIO_PORTE_AMSEL_R |= AIN3_MASK;                 // turn on analog operation on pin PE0
}

//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------

int main(void)
{
    uint16_t raw;
    uint16_t x[16];
    float instantTemp, firTemp, iirTemp = 0;
    uint8_t index = 0;
    uint16_t sum = 0; // total fits in 16b since 12b adc output x 16 samples
    uint8_t i;
    float alpha = 0.80;
    char str[100];

    // Initialize hardware
    initHw();
    initUart0();
    initAdc0Ss3();

    // Setup UART0 baud rate
    setUart0BaudRate(115200, 40e6);

    // Use AIN3 input with N=4 hardware sampling
    setAdc0Ss3Mux(3);
    setAdc0Ss3Log2AverageCount(2);

    // Clear FIR filter taps
    for (i = 0; i < 16; i++)
        x[i] = 0;

    // Endless loop
    while(true)
    {
        // Read sensor
        raw = readAdc0Ss3();

        // Calculate temperature in degC as follows:
        //   For the 12-bit SAR ADC with Vref+ = 3.3V and Vref- = 0V, outputing a result R:
        //   Resolution is approx 0.81mV / LSb or 0.13 degC / LSb
        //   R(Vin) = floor(Vin/3.3V * 4096) -> Vin(R) ~= 3.3V * ((R+0.5) / 4096)
        //   (~ and 0.5LSb offset in Vin(R) equation are introduced for mid-tread value of the SAR transfer function)
        //   T(Vin) = (Vin - 0.424V) / 0.00625V
        //   T(R) ~= ([3.3V * ((R+0.5) / 4096)] - 0.424V) / 0.00625V
        //   T(R) ~= (0.12890625 * R) - 67.775546875 (simplified floating point equation to save cycles)
        instantTemp = (((raw+0.5) / 4096.0 * 3.3) - 0.424) / 0.00625;

        // FIR sliding average filter with circular addressing
        sum -= x[index];
        sum += raw;
        x[index] = raw;
        index = (index + 1) % 16;
        firTemp = ((((sum/16.0)+0.5) / 4096.0 * 3.3) - 0.424) / 0.00625;

        // IIR filtering of first order, alpha = 0.90
        //   In the z-domain:
        //     H(z) = sum(j = 0..M) {bj * z^(-j)
        //            --------------------------
        //            sum(i = 0..N) {ai * z^(-i)
        //   Setting a0 = 1, yields:
        //     H(z) = sum(j = 0..M) {bj * z^(-j)
        //            ------------------------------
        //            1 + sum(i = 1..N) {ai * z^(-i)
        //   for N = 1, M = 0:
        //     H(z) = b0 / [1 + a1 * z^(-1)]
        //   Given IIR difference equation:
        //     sum(i = 0..N) {ai * y(n-i)} = sum(j = 0..M) {bj * x(n-j)}
        //   Separating y(n), rearranging and inverting signs of a(1-N), yields
        //     a0 * y(n) = sum(i = 1..N) {ai * y(n-i)} + sum(j = 0..M) {bj * x(n-j)}
        //     for N = 1, M = 0, and a0 = 1,
        //       y(n) = b0 * x(n) + a1 * y(n-1)
        //   Setting b0 = (1-a1), yields
        //     y(n) = alpha * y(n-1) + (1-alpha) * x(n)
        //   Adding an exception for the first sample, yields:
        //     y(n) = x(n); n = 0
        //     y(n) = alpha * y(n-1) + (1-alpha) * x(n); n > 0
        iirTemp = alpha * iirTemp + (1-alpha)*instantTemp;

        // display raw ADC value and temperatures
        sprintf(str, "Raw ADC:          %4u\r\n", raw);
        putsUart0(str);
        sprintf(str, "Filtered Raw ADC: %4u\r\n", sum / 16);
        putsUart0(str);
        sprintf(str, "Index:              %2u\r\n", index);
        putsUart0(str);
        sprintf(str, "Unfiltered (C):   %4.1f\r\n", instantTemp);
        putsUart0(str);
        sprintf(str, "FIR filtered (C): %4.1f\r\n", firTemp);
        putsUart0(str);
        sprintf(str, "IIR filtered (C): %4.1f\r\n", iirTemp);
        putsUart0(str);
        putsUart0("\r\n");

        waitMicrosecond(1000000);
    }
}
