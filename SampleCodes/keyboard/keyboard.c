// Keyboard Example
// Jason Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL Evaluation Board
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

// Hardware configuration:
// Red LED:
//   PF1 drives an NPN transistor that powers the red LED
// Green LED:
//   PF3 drives an NPN transistor that powers the green LED
// UART Interface:
//   U0TX (PA1) and U0RX (PA0) are connected to the 2nd controller
//   The USB on the 2nd controller enumerates to an ICDI interface and a virtual COM port
//   Configured to 115,200 baud, 8N1
// 4x4 Keyboard
//   Column 0-3 open drain outputs on PB0, PB1, PB4, PA6
//   Rows 0-3 inputs with pull-ups on PE1, PE2, PE3, PA7
//   To locate a key (r, c), the column c is driven low so the row r reads low

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "tm4c123gh6pm.h"
#include "wait.h"
#include "uart0.h"
#include "kb.h"

// Bitband aliases
#define RED_LED      (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 1*4)))
#define GREEN_LED    (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 3*4)))

// PortF masks
#define GREEN_LED_MASK 8
#define RED_LED_MASK 2

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
    SYSCTL_RCGCGPIO_R = SYSCTL_RCGCGPIO_R5;
    _delay_cycles(3);

    // Configure LEDs
    GPIO_PORTF_DIR_R |= GREEN_LED_MASK | RED_LED_MASK;   // bits 1 and 3 are outputs
    GPIO_PORTF_DR2R_R |= GREEN_LED_MASK | RED_LED_MASK; // set drive strength to 2mA (not needed since default configuration -- for clarity)
    GPIO_PORTF_DEN_R |= GREEN_LED_MASK | RED_LED_MASK;  // enable LEDs
}

//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------

int main(void)
{
	// Initialize hardware
	initHw();
	initUart0();
	initKb();

    // Setup UART0 baud rate
    setUart0BaudRate(115200, 40e6);

    // Power-on flash
	GREEN_LED = 1;
	waitMicrosecond(250000);
	GREEN_LED = 0;
	waitMicrosecond(250000);

	// Display greeting
    putsUart0("Keyboard Example\n");
    putsUart0("Press '0' or '1' to turn LED on and off\n");

    // Poll keyboard
	char c;
    while(true)
    {
    	// Send characters to UART0 if available
		if (kbhit())
		{
			RED_LED ^= 1;
			c = getKey();
			if (c != 'D')
                putcUart0(c);
			else
				putsUart0("\n");
		}

		// Check UART0 for data
		if (kbhitUart0())
		{
            char c = UART0_DR_R & 0xFF;
            if (c == '1')
                GREEN_LED = 1;
            if (c == '0')
                GREEN_LED = 0;
		}

	    // Emulate a foreground task that is running continuously
		waitMicrosecond(250000);
    }
}
