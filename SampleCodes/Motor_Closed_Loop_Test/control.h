
#ifndef CONTROL_H_
#define CONTROL_H_

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "inc/hw_i2c.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_pwm.h"
#include "inc/hw_adc.h"
#include "inc/hw_ints.h"
#include "inc/hw_qei.h"
#include "driverlib/i2c.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/interrupt.h"
#include "driverlib/pwm.h"
#include "driverlib/qei.h"
#include "driverlib/adc.h"
#include "driverlib/interrupt.h"
#include "driverlib/comp.h"
#include "driverlib/fpu.h"
#include "driverlib/timer.h"


#define DIR_CCW 1
#define DIR_CW 0
#define PWM_PERIOD (FCY/(PWM_FREQ))
#define PWM_LOAD ((FCY/(PWM_FREQ))/2)-1 //Used for loading the CMP Register (For up/down mode, this value is half the period)
#define PWM_FREQ 31e3 //PWM Frequency 50KHz
#define FCY 40e6 //Clock Frequency 40MHz
#define MAX_PWM_LOAD 516 //This sets the max PWM at 90%
#define MAX_PWM 375
#define MIN_PWM 337

#define RUNNING 1
#define STOPPED 0

//Output PWM bits (PWM_OUT_X_BIT)
#define PWM0_AH 1
#define PWM1_AL 2
#define PWM2_BH 4
#define PWM3_BL 8
#define PWM4_CH 16
#define PWM5_CL 32

#define PWM0 64 //PB6 PWMAH
#define PWM1 128 //PB7 PWMAL
#define PWM2 16 //PB4 PWMBH
#define PWM3 32 //PB5 PWMBL
#define PWM4 16 //PE4 PWMCH
#define PWM5 32 //PE5 PWMCL

// ADC Variables

#define MTR_1_I_SENSE   0x00000000  // Input channel 0, PE3
#define POSITION_POT   0x00000001  // Input channel 1, PE2
#define VBUS_SENSE      0x00000002  // Input channel 2, PE1
#define BAT_V_SENSE     0x00000003  // Input channel 2, PE0
#define BAT_I_SENSE     0x00000004  // Input channel 2, PD3

extern bool motor1_update_flag;
extern bool motor1_position_update_flag;
extern bool battery_update_flag;
extern uint16_t ADC0SS0_Raw[3];
extern uint16_t ADC0SS1_Raw;
extern uint16_t ADC0SS2_Raw;

extern uint8_t Motor0_Direction;
extern uint8_t Motor0_Enable, Motor0_status;
extern uint16_t PWM0_Duty;

void PWM0_OutputUpdate(uint8_t Enable);
void setPWM0_Duty(uint32_t Duty);
void Start_Motor0(void);
void Stop_Motor0(void);
void init_Motor0(void);

void PORTAISR(void);
void PORTFISR(void);

void initADC();
void ADC0_Seq0_ISR(void);
void ADC0_Seq1_ISR(void);
void ADC0_Seq2_ISR(void);

void initFPU();
void initHW();
void initPWM();
void initQEI();
void initADC();
void initTimer();
void initComparator();
void initQEI();


#endif /* CONTROL_H_ */

