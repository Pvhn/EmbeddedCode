// Stop Go C Example (Masking)
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
// Pushbutton:
//   SW1 pulls pin PF4 low (internal pull-up is used)

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"

// Masked port
#define LEDS         (*((volatile uint32_t *)(0x40025000 + 0x0E*4))) //Only for Red and Green

// Bitband alias
#define SW1  (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 4*4)))

// PortF masks
#define GREEN_LED_MASK 8
#define BLUE_LED_MASK 4
#define RED_LED_MASK 2
#define SW1_MASK 16

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// Blocking function that returns only when SW1 is pressed
void waitPbPress()
{
	while(SW1);
}

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

    // Configure LED and pushbutton pins
    GPIO_PORTF_DIR_R |= GREEN_LED_MASK | RED_LED_MASK | BLUE_LED_MASK;   // bits 1 and 3 are outputs, other pins are inputs
    GPIO_PORTF_DIR_R &= ~SW1_MASK;               // bit 4 is an input
    GPIO_PORTF_DR2R_R |= GREEN_LED_MASK | RED_LED_MASK |BLUE_LED_MASK;  // set drive strength to 2mA (not needed since default configuration -- for clarity)
    GPIO_PORTF_DEN_R |= SW1_MASK | GREEN_LED_MASK | RED_LED_MASK | BLUE_LED_MASK;
                                                         // enable LEDs and pushbuttons
    GPIO_PORTF_PUR_R |= SW1_MASK;                // enable internal pull-up for push button
}

//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------

int main(void)
{
	// Initialize hardware
	initHw();

    // Endless loop
    while(true)
    {
        LEDS = 1;

        // Wait for PB press
        waitPbPress();

        // Turn off red LED, turn on green LED, other port bits are unmodified
        LEDS = BLUE_LED_MASK | RED_LED_MASK | GREEN_LED_MASK;
        waitPbPress();
    }
}
