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
    // Configure HW to work with 16 MHz XTAL, PLL enabled, sysdivider of 5, creating system clock of 40 MHz
    SYSCTL_RCC_R = SYSCTL_RCC_XTAL_16MHZ | SYSCTL_RCC_OSCSRC_MAIN | SYSCTL_RCC_USESYSDIV | (4 << SYSCTL_RCC_SYSDIV_S);

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
void waitMicrosecond(uint32_t us)
{
    __asm("WMS_LOOP0:   MOV  R1, #6");          // n
    __asm("WMS_LOOP1:   SUB  R1, #1");          // 6n
    __asm("             CBZ  R1, WMS_DONE1");   // 5n+n*3
    __asm("             NOP");                  // 5n
    __asm("             NOP");                  // 5n
    __asm("             B    WMS_LOOP1");       // 5n*2 (speculative, so P=1)
    __asm("WMS_DONE1:   SUB  R0, #1");          // n
    __asm("             CBZ  R0, WMS_DONE0");   // (n-1)+3
    __asm("             NOP");                  // (n-1)
    __asm("             B    WMS_LOOP0");       // (n-1)*2
    __asm("WMS_DONE0:");                        // ---
                                                // 40 clocks/us + error
}

//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------

int main(void)
{
	// Initialize hardware
	initHw();
	RED_LED = 0;
	BLUE_LED = 1;
    // Toggle red LED every second
	while(true)
	{
      RED_LED ^= 1;
      BLUE_LED ^= 1;
      waitMicrosecond(80000);
	}
}
