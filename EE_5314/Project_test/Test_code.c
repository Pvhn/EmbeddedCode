// EE 5314 FA2020 Hardware Test Code
// Peter Nguyen

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
#define ALL_LEDS (*((volatile uint32_t *)(0x40025000 + 0x0E*4))) // Mask for all LEDs

// Bitband aliases
#define SW1 (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 4*4)))
#define RED_LED (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 1*4)))
#define BLUE_LED (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 2*4)))
#define GREEN_LED (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 3*4)))
#define PC4 (*((volatile uint32_t *)(0x42000000 + (0x400063FC-0x40000000)*32 + 4*4)))
#define PC5 (*((volatile uint32_t *)(0x42000000 + (0x400063FC-0x40000000)*32 + 5*4)))
#define PC7 (*((volatile uint32_t *)(0x42000000 + (0x400063FC-0x40000000)*32 + 7*4)))

// PortF masks
#define GREEN_LED_MASK 8
#define BLUE_LED_MASK 4
#define RED_LED_MASK 2
#define SW1_MASK 16

// PortC mask
#define PC4_MASK 16
#define PC5_MASK 32
#define PC7_MASK 128

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// Blocking function that returns only when SW1 is pressed
void waitPbPress()
{
    while(SW1);
}

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
// Initialize Hardware
void initHw()
{
    // Configure HW to work with 16 MHz XTAL, PLL enabled, sysdivider of 5, creating system clock of 40 MHz
    SYSCTL_RCC_R = SYSCTL_RCC_XTAL_16MHZ | SYSCTL_RCC_OSCSRC_MAIN | SYSCTL_RCC_USESYSDIV | (4 << SYSCTL_RCC_SYSDIV_S);

    // Set GPIO ports to use APB (not needed since default configuration -- for clarity)
    SYSCTL_GPIOHBCTL_R = 0;

    // Enable clocks
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R5 |SYSCTL_RCGCGPIO_R2;
    _delay_cycles(3);
    // Configure Ports
    //Outputs
    GPIO_PORTC_DIR_R |= PC5_MASK |PC7_MASK;
    GPIO_PORTF_DIR_R |= GREEN_LED_MASK | RED_LED_MASK |BLUE_LED_MASK; //Bits 1,2, and 3 are outputs
    //Inputs
    GPIO_PORTC_DIR_R &= ~PC4_MASK; //PortC4
    GPIO_PORTF_DIR_R &= ~SW1_MASK; //SW1 (PortF4)
    //Setting Drive Strength
    GPIO_PORTF_DR2R_R |= GREEN_LED_MASK | RED_LED_MASK |BLUE_LED_MASK;  // set drive strength to 2mA (not needed since default configuration -- for clarity)
    //Digital Enable
    GPIO_PORTC_DEN_R |= PC5_MASK | PC7_MASK | PC4_MASK;
    GPIO_PORTF_DEN_R |= SW1_MASK | GREEN_LED_MASK | RED_LED_MASK | BLUE_LED_MASK;
    //Internal Resistors
    GPIO_PORTF_PUR_R |= SW1_MASK;   // enable internal pull-up for push button
    //GPIO_PORTC_PDR_R |= PC4_MASK;
}
//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------

int main(void)
{
    // Initialize hardware
    initHw();
    while(true)
    {
        //Stage 1
        LEDS = RED_LED_MASK;
        PC5 = 1;
        PC7 = 1;
        waitMicrosecond(2000000); //Wait 1 second
        //Stage 2
        LEDS = BLUE_LED_MASK;
        PC5 = 0;
        PC7 = 1;
        waitMicrosecond(2000000); //Wait 1 second
        //Stage 3
        LEDS = GREEN_LED_MASK;
        PC5 = 1;
        PC7 = 0;
        waitMicrosecond(2000000); //Wait 1 second
        //Stage 4
        LEDS = RED_LED_MASK | GREEN_LED_MASK;
        PC5 = 0;
        PC7 = 0;
        waitPbPress();
        waitMicrosecond(1000000); //Wait 5 milliseconds
        //Stage 5
        PC5 = 0;
        PC7 = 0;
        while(SW1)
        {
            if(PC4 ==1)
            {
                LEDS = RED_LED_MASK| BLUE_LED_MASK;
            }
            else
            {
                LEDS = 1;
            }
        }
        waitMicrosecond(1000000); //Wait 5 milliseconds
        while(SW1)
        {
            if(PC4 ==0)
            {
                LEDS = GREEN_LED_MASK| BLUE_LED_MASK;
            }
            else
            {
                LEDS = 0;
            }
        }
        waitMicrosecond(1000000); //Wait 5 milliseconds

        //waitPbPress();
    }
}
