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


/*========================================================
 * Preprocessor Defintions
 *========================================================
 */
// Memory Alias for PORTF1
#define HBLED (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 2*2)))
#define HBLED_MASK 0x02
#define DAC_SIZE 64 // DAC Size of 6-bits

/*========================================================
 * Variable Declarations
 *========================================================
 */
extern const uint32_t Notes[16];
extern volatile uint32_t play;
extern volatile uint32_t SineWave[DAC_SIZE*2];

/*========================================================
 * Function Declarations
 *========================================================
 */
extern void Sound_Init(void);
extern void Sound_Play(uint32_t frequency);
extern void Sound_Stop(void);

#endif /* SOUND_H_ */
