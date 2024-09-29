/*
 * DAC.c
 *
 *  Created on: Sep 28, 2024
 *      Author: peter
 */

#include "DAC.h"

void DAC_Init(void)
{
    // Enable GPIO PORTB
    SYSCTL_RCGCGPIO_R |= 0x0002;
    _delay_cycles(3);

    GPIO_PORTB_DIR_R |= DAC_MASK;     // Enable PB0-5 as outputs
    GPIO_PORTB_DEN_R |= DAC_MASK;     // Set Digital Enable for PB0-5
    GPIO_PORTB_DR2R_R |= DAC_MASK;    // Set Drive Strength to 2mA (default)
}

void DAC_Out(uint32_t data)
{
    // The data is mapped as is to each of the 6 bits
    DAC_OUT = data;
}
