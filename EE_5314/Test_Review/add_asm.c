
#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"

// Bitband alias
#define RED_LED      (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 1*4)))
#define SW1 (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 4*4)))

// PortF masks
#define RED_LED_MASK 2
#define SW1_MASK 16

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
    GPIO_PORTF_DIR_R &= ~SW1_MASK;
    //GPIO_PORTF_DR2R_R |= RED_LED_MASK|BLUE_LED_MASK; // set drive strength to 2mA (not needed since default configuration -- for clarity)
   GPIO_PORTF_DEN_R |= SW1_MASK;
    GPIO_PORTF_PUR_R |= SW1_MASK;   // enable internal pull-up for push button
}

// Approximate busy waiting (in units of microseconds), given a 40 MHz system clock
uint32_t sum4(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
    __asm(" ADD R0, R1");
    __asm(" ADD R0, R2");
    __asm(" ADD R0, R3");
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

//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------

int main(void)
{
    // Initialize hardware
    initHw();
    uint32_t sum,a,b,c,d;
    a = 0;
    b = 2;
    c = 2;
    d = 1;
    // Toggle red LED every second
    while(true)
    {
        sum = sum4(a,b,c,d);
      if(SW1==0)
      {

          a +=1;
          waitMicrosecond(5000);
      }
    }
}
