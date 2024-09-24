/*
 * Utilities.c
 *
 *  Created on: Sep 7, 2024
 *      Author: peter
 */

#include "Utilities.h"

/*========================================================
 * Variable Definitions
 *========================================================
 */

// Traffic Light Control State Machine Array
StatesEnumType state_arr[9][8] =
{
 {SouthG,SouthG, SouthY, SouthY, SouthY, SouthY, SouthY, SouthY},
 {WestG, WestY, WestG, WestY, WestY, WestY, WestY, WestY},
 {PedG, PedY, PedY, PedY, PedG, PedY, PedY, PedY},
 {SouthR, SouthR, SouthR, SouthR, SouthR, SouthR, SouthR, SouthR},
 {WestR, WestR, WestR, WestR, WestR, WestR, WestR, WestR},
 {PedR, PedR, PedR, PedR, PedR, PedR, PedR, PedR},
 {WestG, SouthG, WestG, WestG, PedG, PedG, WestG, WestG},
 {PedG, SouthG, WestG, SouthG, PedG, PedG, PedG, PedG},
 {SouthG, SouthG, WestG, SouthG, PedG, SouthG, WestG, SouthG},
};

/*========================================================
 * Function Definitions
 *========================================================
 */

/*=======================================================
 * Function Name: initHW
 *=======================================================
 * Parameters: None
 * Description:
 * This function performs the initialization of
 * all system peripherals including the system
 * clock and all GPIO ports.
 *=======================================================
 */
void initHW()
{
    // Configure HW to work with 16 MHz XTAL, PLL enabled, system clock of 40 MHz
    SYSCTL_RCC_R = SYSCTL_RCC_XTAL_16MHZ | SYSCTL_RCC_OSCSRC_MAIN | SYSCTL_RCC_USESYSDIV | (4 << SYSCTL_RCC_SYSDIV_S);

    // Set GPIO ports to use APB (not needed since default configuration -- for clarity)
    SYSCTL_GPIOHBCTL_R = 0;

    // Enable Timer and GPIO PORTB,E, and F
    SYSCTL_RCGCGPIO_R |= 0x0032;
    _delay_cycles(3);

    // Initialize SysTick Timer
    NVIC_ST_CTRL_R = 0; // Ensure SysTick is disabled before config
    NVIC_ST_CTRL_R = 0x0005; // Use the core clock SysTick

    GPIO_PORTB_DIR_R |= TRAFLED_MASK;     // Enable PB0-5 as outputs
    GPIO_PORTB_DEN_R |= TRAFLED_MASK;     // Set Digital Enable for PB0-5
    GPIO_PORTB_DR2R_R |= TRAFLED_MASK;    // Set Drive Strength to 2mA (default)

    GPIO_PORTE_DIR_R &= ~SENSOR_MASK;   // Enable PE1,2,3 as inputs
    GPIO_PORTE_DEN_R |= SENSOR_MASK;    // Set Digital Enable
    GPIO_PORTE_PDR_R |= SENSOR_MASK;    // Set internal Pull-Down Resistors

    GPIO_PORTF_DIR_R |= PEDLED_MASK;     // Enable PF1,2,3 as outputs
    GPIO_PORTF_DEN_R |= PEDLED_MASK;     // Set Digital Enable
    GPIO_PORTF_DR2R_R |= PEDLED_MASK;    // Set Drive Strength to 2mA (default)
}

/*=======================================================
 * Function Name: SysTick_Delay10ms
 *=======================================================
 * Parameters: delay
 * Description:
 * This function performs a delay at 10ms intervals using
 * the SysTick Timer.
 *=======================================================
 */
void SysTick_Delay10ms(uint32_t delay)
{
    // Number of clock cycles per 10ms
    // Calculated as 10ms* Clock Frequency. (Our clock is 40MHZ)
    const uint32_t ticks = 400000;
    uint32_t time;

    for(time = 0; time < delay; time++)
    {
        // Load the SysTick Timer and clear it
        NVIC_ST_RELOAD_R = ticks - 1;
        NVIC_ST_CURRENT_R = 0;

        // Wait for SysTick to count down
        while((NVIC_ST_CTRL_R & CNT_MASK)==0);
    }
}

/*=======================================================
 * Function Name: TrafficLightControl
 *=======================================================
 * Parameters: state
 * Returns: refresh
 * Description:
 * This function is the primary controller for sequencing
 * the traffic lights. A state input is provided of
 * which the appropriate lights will be set for the
 * requested state.
 *=======================================================
 */
void TrafficLightControl(uint32_t state)
{
    uint16_t cycle = 0;
    uint32_t delay = 100;

    switch(state)
    {
    // South Traffic Green State
    case SouthG:
        TRAF_LEDS = SOUTHG | WESTR;   // Set South G and West R. All others off.
        PED_LEDS = PEDR;    // Set Pedestrian Don't Walk Light.
        delay = 100;
        break;
    // West Traffic Green State
    case WestG:
        TRAF_LEDS = SOUTHR | WESTG;   // Set West Green
        PED_LEDS = PEDR;    // Set Pedestrian Don't Walk Light.
        delay = 100;
        break;
    // Pedestrian Walk State
    case PedG:
        TRAF_LEDS = SOUTHR| WESTR;   // Set West and South Red.
        PED_LEDS = PEDG;    // Set Pedestrian Walk Light
        delay = 100;
        break;
    case SouthY:
        TRAF_LEDS = SOUTHY | WESTR; // Set South Yellow and West Red
        PED_LEDS = PEDR;    // Set Pedestrian Don't Walk Light
        delay = 100;
        break;
    case WestY:
        TRAF_LEDS = SOUTHR | WESTY; // Set South Red, West Yellow
        PED_LEDS = PEDR;    // Set Pedestrian Don't Walk Light
        delay = 100;
        break;
    case PedY:
        TRAF_LEDS = SOUTHR | WESTR; // Set South and West Red

        // Flash the Pedestrian Red Light 3 times.
        for(cycle = 0; cycle <3; cycle++)
        {
            // Flashing is performed at a rate of 0.5s
            PED_LEDS = 0x0;
            SysTick_Delay10ms(50);
            PED_LEDS = PEDR;
            SysTick_Delay10ms(50);
        }
        break;
    case SouthR:
    case WestR:
    case PedR:
        TRAF_LEDS = SOUTHR | WESTR; // Set South and West Red
        PED_LEDS = PEDR;    // Set Pedestrian Don't Walk Light
        delay = 100;
        break;
    default:
        break;
    }

    // Skip system delay for Pedestrian Transition
    if(state != PedY)
    {
        // System Delay
        SysTick_Delay10ms(delay);
    }
}

