/*
 * DAC.c
 *
 *  Created on: Sep 28, 2024
 *      Author: peter
 */

#include "DAC.h"

/*========================================================
 * Function Definitions
 *========================================================
 */

/*=======================================================
 * Function Name: DAC_Init
 *=======================================================
 * Parameters: None
 * Returns: None
 * Description:
 * This function initializes the Port B pins 0-5 for
 * use as 6-bit DAC Output.
 *========================================================*/
void DAC_Init(void)
{
    // Enable GPIO PORTB
    SYSCTL_RCGCGPIO_R |= 0x0002;
    _delay_cycles(3);

    GPIO_PORTB_DIR_R |= DAC_MASK;     // Enable PB0-5 as outputs
    GPIO_PORTB_DEN_R |= DAC_MASK;     // Set Digital Enable for PB0-5
    GPIO_PORTB_DR2R_R |= DAC_MASK;    // Set Drive Strength to 2mA (default)
}

/*=======================================================
 * Function Name: DAC_Out
 *=======================================================
 * Parameters: None
 * Returns: None
 * Description:
 * This function sets the DAC Output based on the
 * provided input data. Note: DAC_OUT is a bit-band alias
 * for Port B pins 0-5.
 *========================================================*/
void DAC_Out(uint32_t data)
{
    // The data is mapped as is to each of the 6 bits
    DAC_OUT = data;
}

void DAC_Test(void)
{
    DAC_OUT = 0;
    DAC_OUT = 1;
    DAC_OUT = 7;
    DAC_OUT = 8;
    DAC_OUT = 15;
    DAC_OUT = 16;
    DAC_OUT = 17;
    DAC_OUT = 18;
    DAC_OUT = 31;
    DAC_OUT = 32;
    DAC_OUT = 33;
    DAC_OUT = 47;
    DAC_OUT = 48;
    DAC_OUT = 49;
    DAC_OUT = 62;
    DAC_OUT = 63;
}
