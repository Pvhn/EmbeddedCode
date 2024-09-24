/*
 * main.c
 *
 *Created on: Sep 7, 2024
 *Author: peter
 */

#include "Utilities.h"

int main(void)
{
    initHW();

    uint32_t input = 0;
    uint32_t state = 0;
    while(1)
    {
        // Perform Traffic Light Sequencing based on state
        TrafficLightControl(state);

        // Read Traffic Sensors (PE1,2,3) for new inputs.
        // Note: Value is shifted right by one bit so
        // that the input begins at 0 for indexing.
        input = (SENSORS >> 1);

        // Set the next transition state based
        // on the state of the sensors
        state = state_arr[state][input];
    }
}
