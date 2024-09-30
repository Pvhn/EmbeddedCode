/*
 * Keys.c
 *
 *  Created on: Sep 28, 2024
 *      Author: peter
 */

#include "Keys.h"

/*========================================================
 * Variable Definitions
 *========================================================
 */
// Input used to store state of key presses
volatile uint32_t keys = 0;

/*========================================================
 * Function Definitions
 *========================================================
 */

/*=======================================================
 * Function Name: Key_Init
 *=======================================================
 * Parameters: None
 * Returns: None
 * Description:
 * This function initializes the GPIO ports for the
 * key inputs. The function also will set-up and enable
 * key press interrupts for PortE
 *========================================================*/
void Key_Init(void)
{
    // Enable PORTE Peripheral
    SYSCTL_RCGCGPIO_R |= 0x0010;
    _delay_cycles(3);

    GPIO_PORTE_DIR_R &= ~KEYS_MASK;   // Enable PE0,1,2,3 as inputs
    GPIO_PORTE_DEN_R |= KEYS_MASK;    // Set Digital Enable
    GPIO_PORTE_PDR_R |= KEYS_MASK;    // Set internal Pull-Down  Resistors

    // Configuring GPIO Interrupts
    GPIO_PORTE_IM_R &= ~KEYS_MASK;       // Mask out interrupts to prevent false trip during config
    GPIO_PORTE_IS_R &= ~KEYS_MASK;       // Set for interrupt on edge detect
    GPIO_PORTE_IBE_R |= KEYS_MASK;      // Clear IBE to use only single edge detect
//    GPIO_PORTE_IEV_R |= KEYS_MASK;      // Set Keys to interrupt on rising edge
    GPIO_PORTE_ICR_R |= KEYS_MASK;       // Clear any pending Interrupts
    GPIO_PORTE_IM_R |= KEYS_MASK;        // Turn on Switch Interrupts
    NVIC_EN0_R |= 1 << (INT_GPIOE-16);   // Enable interrupts for Port E
}

/*=======================================================
 * Function Name: Key_In
 *=======================================================
 * Parameters: None
 * Returns: KEYS
 * Description:
 * This function simply returns a read of the Port E
 * pins 0-3 using the KEYS bit-band alias.
 *========================================================
 */
uint32_t Key_In(void)
{
    // Returns the Data Register Read of PortE0-3
    return KEYS;
}

/*=======================================================
 * Function Name: PortEISR
 *=======================================================
 * Parameters: None
 * Returns: None
 * Description:
 * This interrupt service routine handles key press
 * interrupts on Port E. Since multiple key presses can
 * trigger the Port E Interrupt, the Masked Interrupt
 * Status(MIS) must be saved at the beginning of the ISR.
 * This allows for the ISR to clear only the interrupt
 * for the key(s) being serviced in the current iteration.
 * The ISR will perform a call to Keys_In function to
 * perform a read of the keys pressed.
 *========================================================
 */
void PortEISR(void)
{
    // Save off current Interrupt Status
    uint32_t MIS_R = GPIO_PORTE_MIS_R & KEYS_MASK;

    // Read Keys pressed
    keys = Key_In();

    // Clear the interrupt(s) for the pin(s) serviced
    GPIO_PORTE_ICR_R |= MIS_R;
}
