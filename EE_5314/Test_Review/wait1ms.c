// Timing C/ASM Mix Example
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

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"

// Bitband alias
#define RED_LED      (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 1*4)))
#define BLUE_LED      (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 2*4)))

// PortF masks
#define RED_LED_MASK 2
#define BLUE_LED_MASK 4
//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// Initialize Hardware
void initHw()
{
    // Configure HW to work with 16 MHz XTAL, PLL enabled, sysdivider of 5, creating system clock of 20 MHz
    SYSCTL_RCC_R = SYSCTL_RCC_XTAL_16MHZ | SYSCTL_RCC_OSCSRC_MAIN | SYSCTL_RCC_USESYSDIV | (9 << SYSCTL_RCC_SYSDIV_S);

    // Set GPIO ports to use APB (not needed since default configuration -- for clarity)
    SYSCTL_GPIOHBCTL_R = 0;

    // Enable clocks
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R5;
    _delay_cycles(3);

    // Configure LED pin
    GPIO_PORTF_DIR_R |= RED_LED_MASK|BLUE_LED_MASK;  // make bit an output
    GPIO_PORTF_DR2R_R |= RED_LED_MASK|BLUE_LED_MASK; // set drive strength to 2mA (not needed since default configuration -- for clarity)
    GPIO_PORTF_DEN_R |= RED_LED_MASK|BLUE_LED_MASK;  // enable LED
}

// Approximate busy waiting (in units of microseconds), given a 40 MHz system clock
void wait1ms(void)
{
    __asm("             MOV  R1, #4000");          // n
    __asm("WMS_LOOP:   SUB  R1, #1");          // 6n
    __asm("             CBZ  R1, WMS_DONE");   // 5n+n*3
    __asm("             NOP");                  // 5n
    __asm("             B    WMS_LOOP");       // 5n*2 (speculative, so P=1)
    __asm("WMS_DONE:");                        // ---
                                                // 40 clocks/us + error
}

//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------

int main(void)
{
	// Initialize hardware
	initHw();
    // Toggle red LED every second
	while(true)
	{
      RED_LED ^= 1;
      wait1ms();
	}
}
