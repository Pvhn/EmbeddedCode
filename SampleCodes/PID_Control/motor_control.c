// Motor Control Library
// Jason Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL
// Target uC:       TM4C123GH6PM
// System Clock:    -

// Hardware configuration:
// Servo motor drive:
//   PWM output on M1PWM6 (PF2) - blue on-board LED
//   DIR output on PF3 - green on-board LED

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"
#include "motor_control.h"

// Bitband aliases
#define DIRECTION    (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 3*4)))

// PortF masks
#define PWM_MASK 4
#define DIRECTION_MASK 8

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// Initialize motor control
void initMotorControl()
{
    // Enable clocks
    SYSCTL_RCGCPWM_R |= SYSCTL_RCGCPWM_R1;
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R5;
    _delay_cycles(3);


    // Configure PWM and DIRECTION outputs
    GPIO_PORTF_DIR_R |= PWM_MASK | DIRECTION_MASK;   // make bits 2 and 3 outputs
    GPIO_PORTF_DR2R_R |= PWM_MASK | DIRECTION_MASK;  // set drive strength to 2mA (not needed since default configuration -- for clarity)
    GPIO_PORTF_DEN_R |= PWM_MASK | DIRECTION_MASK;   // enable outputs
    GPIO_PORTF_AFSEL_R |= PWM_MASK;                  // select auxiliary function for bit 2
    GPIO_PORTF_PCTL_R &= GPIO_PCTL_PF2_M;            // enable PWM on bit 2
    GPIO_PORTF_PCTL_R |= GPIO_PCTL_PF2_M1PWM6;

    // Configure PWM module 1 to drive servo
    // PWM on M1PWM6 (PF2), M0PWM3a
    SYSCTL_SRPWM_R |= SYSCTL_SRPWM_R1;               // reset PWM1 module
    SYSCTL_SRPWM_R &= ~SYSCTL_SRPWM_R1;              // leave reset state
    __asm(" NOP");                                   // wait 3 clocks
    __asm(" NOP");
    __asm(" NOP");
    PWM1_3_CTL_R = 0;                                // turn-off PWM1 generator 3
    PWM1_3_GENA_R = PWM_1_GENA_ACTCMPAD_ZERO | PWM_1_GENA_ACTLOAD_ONE;
    PWM1_3_LOAD_R = 1024;                            // set period to 40 MHz sys clock / 2 / 1024 = 19.53125 kHz
    PWM1_INVERT_R = PWM_INVERT_PWM6INV;
                                                     // invert outputs for duty cycle increases with increasing compare values
    PWM1_3_CMPA_R = 0;                               // PWM off (0=always low, 1023=always high)
    PWM1_3_CTL_R = PWM_1_CTL_ENABLE;                 // turn-on PWM1 generator 3
    PWM1_ENABLE_R = PWM_ENABLE_PWM6EN;               // enable PWM output
}

void setMotorPwm(unsigned int dutyCycle)
{
    PWM1_3_CMPA_R = dutyCycle;
}

void setMotorDirection(bool dir)
{
    DIRECTION = dir;
}
