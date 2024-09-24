/*
 * main.h
 *
 *  Created on: Apr 8, 2021
 *      Author: peter
 */

#ifndef MAIN_H_
#define MAIN_H_

#include <stdint.h>

//global variables
extern int8_t motor_position[2];

extern uint8_t avg_input_power;
extern uint8_t power_generated;

extern uint8_t encoder_1;
extern uint8_t encoder_2;
extern uint8_t hall_sensor_1;
extern uint8_t hall_sensor_2;
extern uint8_t motor_1;
extern uint8_t motor_2;
extern uint8_t micro_controller_1 ;
extern uint8_t micro_controller_2;

extern uint8_t power_supply_1;
extern uint8_t power_supply_2;
extern uint8_t power_supply_3;
extern uint8_t mega_cap_1;
extern uint8_t mega_cap_2;
extern uint8_t breaking_circuit_1;
extern uint8_t breaking_circuit_2;

extern uint8_t motor1pos;
extern uint8_t motor2pos;


extern uint8_t motor_index;
extern uint8_t loop_count;
extern uint8_t func_test_result;

#endif /* MAIN_H_ */
