/*
 * Keys.c
 *
 *  Created on: Sep 28, 2024
 *      Author: peter
 */

#include "Keys.h"

uint32_t keys = 0;
uint32_t update = 0;

void Key_Init(void)
{
    // Enable PORTE Peripheral
    SYSCTL_RCGCGPIO_R |= 0x0010;
    // Enable Timer Peripheral
    SYSCTL_RCGCTIMER_R |= SYSCTL_RCGCTIMER_R1;
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

    // Configuring Timer Interrupt (for debounce)
    TIMER1_CTL_R &= ~TIMER_CTL_TAEN;            // turn-off timer before reconfiguring
    TIMER1_CFG_R = TIMER_CFG_32_BIT_TIMER;      // configure as 32-bit timer (A+B)
    TIMER1_TAMR_R = TIMER_TAMR_TAMR_PERIOD;     // configure for periodic mode (count down)
    TIMER1_TAILR_R = 1000;                    // set load value to 1e3 for 100 Hz interrupt rate
    TIMER1_CTL_R |= TIMER_CTL_TAEN;             // turn-on timer
    NVIC_EN0_R |= 1 << (INT_TIMER1A-16);        // turn-on interrupt 37 (TIMER1A)
}

uint32_t Key_In(void)
{
    return KEYS;
}

void PortEISR(void)
{
    keys = Key_In();

    // Enable Timer Interrupt for debounce
    GPIO_PORTF_IM_R &= ~KEYS_MASK;
    GPIO_PORTF_ICR_R |= KEYS_MASK;
    TIMER1_IMR_R = TIMER_IMR_TATOIM;
}

void Timer1ISR()
{
    static uint8_t debounceCount = 0;

    debounceCount ++;

    // Debounce time is approximately (1/100Hz)*10 = 100ms
    if (debounceCount == 10)
    {
        debounceCount = 0;
        GPIO_PORTE_ICR_R |= KEYS_MASK;       // Clear switch interrupts that occurred during debounce time.
        GPIO_PORTE_IM_R |= KEYS_MASK;        // Re-enable switch interrupt for next press.
        TIMER1_IMR_R &= ~TIMER_IMR_TATOIM;  // Turn off timer interrupt
    }

    // Clear Timer Interrupt flag
    TIMER1_ICR_R = TIMER_ICR_TATOCINT;
}
