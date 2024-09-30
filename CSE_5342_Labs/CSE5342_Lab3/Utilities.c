/*
 * Utilities.c
 *
 *  Created on: Sep 7, 2024
 *      Author: peter
 */

#include "Utilities.h"

/*=======================================================
 * Function Name: System_Init
 *=======================================================
 * Parameters: None
 * Description:
 * This function performs the initialization of
 * all system peripherals including the system
 * clock and all GPIO ports.
 *=======================================================
 */
void System_Init(void)
{
    // Configure HW to work with 16 MHz XTAL, PLL enabled, system clock of 40 MHz
    SYSCTL_RCC_R = SYSCTL_RCC_XTAL_16MHZ | SYSCTL_RCC_OSCSRC_MAIN | SYSCTL_RCC_USESYSDIV | (4 << SYSCTL_RCC_SYSDIV_S);

    // Set GPIO ports to use APB (not needed since default configuration -- for clarity)
    SYSCTL_GPIOHBCTL_R = 0;

    // Enable Timer and GPIO PORTF
    SYSCTL_RCGCGPIO_R |= 0x0020;
    _delay_cycles(3);

    GPIO_PORTF_DIR_R |= 0x02;     // Enable PF1 as an output
    GPIO_PORTF_DEN_R |= 0x02;     // Set Digital Enable
    GPIO_PORTF_DR2R_R |= 0x02;    // Set Drive Strength to 2mA (default)
}

void Test_DAC(void)
{

}
