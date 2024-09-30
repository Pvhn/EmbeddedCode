/*
 * Keys.h
 *
 *  Created on: Sep 28, 2024
 *      Author: peter
 */


#ifndef KEYS_H_
#define KEYS_H_

#include <stdint.h>
#include <tm4c123gh6pm.h>

/*========================================================
 * Preprocessor Defintions
 *========================================================
 */
// Memory Alias for PortE0-2
#define KEYS (*((volatile uint32_t *)0x4002403C))
#define KEYS_MASK 0x0F

/*========================================================
 * Variable Declarations
 *========================================================
 */
extern volatile uint32_t keys;

/*========================================================
 * Function Declarations
 *========================================================
 */
extern void Key_Init(void);
extern uint32_t Key_In(void);

#endif /* KEYS_H_ */
