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
#define GAIN 0.2
#define SCALE 0.66
#define VSUP 5.0

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
    float Voltage,Current, Power = 0;
    float firVoltage = 0;
    float firCurrent, firPower;
    uint8_t index = 0;
    uint16_t sum = 0; // total fits in 16b since 12b adc output x 16 samples
    uint8_t i;
    float alpha = 0.80;
    char str[100];
    float power = 0.0;

    // Initialize hardware
    initHw();
    initUart0();
    initAdc0Ss3();

    // Setup UART0 baud rate
    setUart0BaudRate(115200, 40e6);

    // Use AIN3 input with N=4 hardware sampling
    setAdc0Ss3Mux(2);
    setAdc0Ss3Log2AverageCount(2);
    // Clear FIR filter taps
    for (i = 0; i < 16; i++)
        x[i] = 0;

    // Endless loop
    while(true)
    {
        // Read sensor
        raw = readAdc0Ss3();
        sum -= x[index];
        sum += raw;
        x[index] = raw;
        index = (index + 1) % 16;
        firVoltage = (((sum/16.0)+0.5) / 4096.0 * 3.3)/SCALE;
        Voltage = ((raw+0.5)/4096.0*3.3)/SCALE;
        Current = (Voltage-(VSUP/2))/GAIN;
        Power = 48*Current; //This is the parameter that is sent to the phyCORE
        firCurrent = (firVoltage-(VSUP/2))/GAIN;
        firPower = 48*firCurrent;
        // display raw ADC value and temperatures
        if(index == 0)
        {
            sprintf(str, "Raw ADC: %4u\r\n", raw);
            putsUart0(str);
            sprintf(str, "Raw Voltage (V): %4.4f\r\n", Voltage);
            putsUart0(str);
            sprintf(str, "Fir Voltage (V): %4.4f\r\n", firVoltage);
            putsUart0(str);
            sprintf(str, "Current (A): %4.4f\r\n", Current);
            putsUart0(str);
            sprintf(str, "Fir Current (A): %4.4f\r\n", firCurrent);
            putsUart0(str);
            sprintf(str, "Power (W): %4.4f\r\n", Power);
            putsUart0(str);
            sprintf(str, "Fir Power (W): %4.4f\r\n", firPower);
            putsUart0(str);
            putsUart0("\r\n");
        }

        waitMicrosecond(10000);
    }
}
