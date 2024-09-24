// QE Example
// Jason Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL with QE
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz
// Stack:           4096 bytes

// Hardware configuration:
// Quadrature Encoder:
//   PhA0 (PD6) and PhB0 (PD7) are connected to 256 cpr optical encoder
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
#include "qei0.h"

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// Initialize Hardware
void initHw()
{
	// Configure HW to work with 16 MHz XTAL, PLL enabled, system clock of 40 MHz
    SYSCTL_RCC_R = SYSCTL_RCC_XTAL_16MHZ | SYSCTL_RCC_OSCSRC_MAIN | SYSCTL_RCC_USESYSDIV | (4 << SYSCTL_RCC_SYSDIV_S);

    // Set GPIO ports to use APB (not needed since default configuration -- for clarity)
    SYSCTL_GPIOHBCTL_R = 0;
}

//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------

int main(void)
{
    int32_t pos;
    char str[80];

    // Initialize hardware
    initHw();
    initUart0();
    setUart0BaudRate(115200, 40e6);
    initQei0();

    while(true)
    {
        // Read sensor
        pos = getQei0Position();
        sprintf(str, "Count: %d\r\nAngle: %d\r\n\n", pos, pos*360/4096);
        putsUart0(str);

        waitMicrosecond(250000);
    }
}
