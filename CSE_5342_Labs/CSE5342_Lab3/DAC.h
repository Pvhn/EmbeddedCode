/*
 * DAC.h
 *
 *  Created on: Sep 28, 2024
 *      Author: peter
 */

#ifndef DAC_H_
#define DAC_H_

#include <tm4c123gh6pm.h>
#include <stdint.h>

#define DAC_OUT (*((volatile uint32_t *)0x400050FC))

#define DAC_MASK 0x3F

extern void DAC_Init(void);

extern void DAC_Out(uint32_t data);


#endif /* DAC_H_ */
