/*
 * Sound.h
 *
 *  Created on: Sep 28, 2024
 *      Author: peter
 */

#ifndef SOUND_H_
#define SOUND_H_

#include <tm4c123gh6pm.h>
#include <math.h>
#include <stdint.h>
#include "DAC.h"

extern uint32_t Notes[16];
extern uint32_t play;

#define DAC_SIZE 64 // DAC Size of 6-bits

// Memory Alias for Traffic, Pedestrian Light Outputs, and Switch Inputs
#define HBLED (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 2*2)))
#define HBLED_MASK 0x02

extern uint32_t SineWave[DAC_SIZE*2];

extern void Sound_Init(void);

extern void Sound_Play(uint32_t frequency);

#endif /* SOUND_H_ */
