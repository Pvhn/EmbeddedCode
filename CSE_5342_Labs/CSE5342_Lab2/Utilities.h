/*
 * Utilities.h
 *
 *  Created on: Sep 7, 2024
 *      Author: peter
 */

#ifndef UTILITIES_H_
#define UTILITIES_H_

#include <stdbool.h>
#include <stdint.h>
#include <tm4c123gh6pm.h>

// Memory Alias for Traffic, Pedestrian Light Outputs, and Switch Inputs
#define TRAF_LEDS (*((volatile uint32_t *)0x400050FC))
#define PED_LEDS (*((volatile uint32_t *)0x40025038))
#define SENSORS (*((volatile uint32_t *)0x40024038))

#define PEDLED_MASK 0x0E
#define TRAFLED_MASK 0x3F
#define SENSOR_MASK 0x0E
#define CNT_MASK 0x00010000

#define WESTR 0x20
#define WESTY 0x10
#define WESTG 0x08
#define SOUTHR 0x4
#define SOUTHY 0x2
#define SOUTHG 0x1
#define PEDR 0x2
#define PEDG 0x8

/*========================================================
 * Variable Declarations
 *========================================================
 */

typedef enum
{
    SouthG,    // 0 - South Green State
    WestG,    // 1 - West Green State
    PedG,    // 2 - Pedestrian Walk State
    SouthY,  // 3 - South Yellow State
    WestY,  // 4 - West Yellow State
    PedY,  // 5 - Pedestrian Warning State
    SouthR,  // 6 - South stop Transition State
    WestR,  // 7 - West stop Transition State
    PedR,  // 8 - Pedestrian stop Transition State
}StatesEnumType;

// State Machine Array consisting of all
// the traffic light system states.
extern StatesEnumType state_arr[9][8];

/*========================================================
 * Function Declarations
 *========================================================
 */
extern void initHW();
extern void SysTick_Delay10ms(uint32_t delay);
extern void TrafficLightControl(uint32_t state);


#endif /* UTILITIES_H_ */
