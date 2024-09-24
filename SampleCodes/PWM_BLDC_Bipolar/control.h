#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_gpio.h"
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "tm4c123gh6pm.h"
#include <driverlib/sysctl.h>
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"
#include "driverlib/pwm.h"
#include "driverlib/qei.h"
#include "driverlib/adc.h"
#include "driverlib/interrupt.h"
#include "driverlib/fpu.h"

#define DIRECTION_CW
#define PWM_PERIOD (FCY/(PWM_FREQ))*2 //Times 2 for up-down mode
#define PWM_FREQ 31e3 //PWM Frequency 50KHz
#define FCY 40e6 //Clock Frequency 40MHz

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
uint32_t Battery_Data[8];
uint32_t Motor_Data[4];
unsigned int Avg_vBUS, Avg_vPOT;

void PWM0_Fault_ISR(void);
void PWM0_ISR(void);

void initADC();
void ADC0_Seq0_ISR(void);
void ADC0_Seq2_ISR(void);

void initHW();
void initTimer();
void initPWM();
void initQEI();
void initADC();

