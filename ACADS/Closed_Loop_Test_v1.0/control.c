
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "inc/hw_gpio.h"
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "tm4c123gh6pm.h"
#include <driverlib/sysctl.h>
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pwm.h"
#include "control.h"
#include "adc.h"
#include "qei.h"


#define DIRECTION    (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 3*4)))
#define FCY 40e6 //Clock Frequency 40MHz
#define PWM_FREQ 40e3 //PWM Frequency 50KHz
#define PWM_PERIOD FCY/(PWM_FREQ) //PWM Period in Clock Ticks
#define DEAD_TIME 5 //Length of Dead band time in Clock Ticks
#define PWM_MIN_DUTY 1 //Minimum Duty Cycle
//static unsigned long PWM0_Period; //Used for later
//uint32_t PWM0_Width;
//uint32_t PWM0_Frequency; //Used for something later
//
//static uint32_t MinPulseWidth;
//static uint8_t PWM0_Flags = 0;
#define PWM_FLAG_NEW_FREQUENCY  0
#define PWM_FLAG_NEW_DUTY_CYCLE 1
#define PWM_FLAG_NEW_PRECHARGE  2

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

//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------

// plant output (y) varies from 0 to 1023 (10 bit range), so place in center
int32_t ySetPoint = 512;

// set step response positions at 1/4 and 3/4 of 10 bit full scale range
int32_t yStep1 = 1024;
int32_t yStep2 = 3072;

// pid calculation of u
int32_t coeffKp = 100;
int32_t coeffKi = 0;
int32_t coeffKd = 0;
int32_t coeffKo = 0;
int32_t coeffK = 100; // denominator used to scale Kp, Ki, and Kd
int32_t integral = 0;
int32_t iMax = 100; //Max value of integral term
int32_t diff;
int32_t error;
int32_t u = 0;
int32_t deadBand = 5;   //If error is less than deadband

// Feedback variables
int32_t y = 0;
uint32_t yDiv = 1;

// Hall Variables
uint8_t Halls, Hall_Int_Status, Prev_Hall_Sequence;
static uint8_t Motor0_Direction = 0; //0 = CW, 1 = CCW
static uint16_t PWM0_DutyCycle;

uint8_t Hall_SequenceCCW[] = { 0, //Fault Condition (All off)
        PWM4_CH|PWM3_BL, // Hall 001
        PWM2_BH|PWM1_AL, // Hall 010
        PWM4_CH|PWM1_AL, // Hall 011
        PWM0_AH|PWM5_CL, // Hall 100
        PWM0_AH|PWM3_BL, // Hall 101
        PWM2_BH|PWM5_CL, // Hall 110
        0 // Fault Condition (All on)
        //Read as Hall 1,2,3 (PA2,3,4) or Hall A,B,C
};

uint8_t Hall_SequenceCW[] = {
        0, //Fault Condition (All off)
        PWM2_BH | PWM5_CL, // Hall 001
        PWM0_AH | PWM3_BL, // Hall 010
        PWM0_AH | PWM5_CL, // Hall 011
        PWM4_CH | PWM1_AL, // Hall 100
        PWM2_BH | PWM1_AL, // Hall 101
        PWM4_CH | PWM3_BL, // Hall 110
        0 // Fault Condition (All on)
};

void initHW()
{
    //Set the clock
    SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ); //40 Mhz
    SysCtlPeripheralEnable (SYSCTL_PERIPH_GPIOA); //Enable Port A
    SysCtlPeripheralEnable (SYSCTL_PERIPH_GPIOE);
    SysCtlDelay(10); // Time for the clock configuration to set

    // Configure PA2 Input pins
    GPIOPinTypeGPIOInput(GPIO_PORTA_BASE, GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4);
    //Configure PortA Interrupts

    IntEnable(INT_GPIOA);
    GPIOIntTypeSet(GPIO_PORTA_BASE, GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4, GPIO_BOTH_EDGES); //Detect rise and fall of hall sensors
    GPIOIntEnable(GPIO_PORTA_BASE, GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4); // Enable interrupt
}

void PORTAISR(void) //Hall Status ISR
{
    GPIOIntClear(GPIO_PORTA_BASE, GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4); //Clear Flag
    Hall_Int_Status = GPIOIntStatus(GPIO_PORTA_BASE, false); //Returns Interrupt Status (DEBUG ONLY)

    //Read Hall inputs. Shifted by 2 to allow value between 0-7
    Halls = (GPIOPinRead(GPIO_PORTA_BASE, GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4)) >> 2;
    if(Motor0_Direction)
    {
        PWM0_OutputUpdate(Hall_SequenceCW[Halls]);
    }
    else
    {
        PWM0_OutputUpdate(Hall_SequenceCCW[Halls]);
    }
}

void Start_Motor(void)
{
    //Read Hall inputs. Shifted by 2 to allow value between 0-7
    Halls = (GPIOPinRead(GPIO_PORTA_BASE, GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4)) >> 2;
    if(Motor0_Direction)
    {
        PWM0_OutputUpdate(Hall_SequenceCW[Halls]);
    }
    else
    {
        PWM0_OutputUpdate(Hall_SequenceCCW[Halls]);
    }
}

//void initPidController()
//{
//    // Enable clocks
//    SYSCTL_RCGCTIMER_R |= SYSCTL_RCGCTIMER_R2;
//    _delay_cycles(3);
//
//    // Configure Timer 2 for PID controller
//    TIMER2_CTL_R &= ~TIMER_CTL_TAEN;                 // turn-off timer before reconfiguring
//    TIMER2_CFG_R = TIMER_CFG_32_BIT_TIMER;           // configure as 32-bit timer (A+B)
//    TIMER2_TAMR_R = TIMER_TAMR_TAMR_PERIOD;          // configure for periodic mode (count down)
//    TIMER2_TAILR_R = 40000;                          // set load value to 40000 for 1000 Hz interrupt rate
//    TIMER2_IMR_R = TIMER_IMR_TATOIM;                 // turn-on interrupts
//    NVIC_EN0_R |= 1 << (INT_TIMER2A-16);             // turn-on interrupt 37 (TIMER1A)
//    TIMER2_CTL_R |= TIMER_CTL_TAEN;                  // turn-on timer
//}
//
//void pidISR()
//{
//    static int32_t errorLast = 0;
//    y = getQei0Position() / yDiv;
//
//    // determine error (desired-actual)
//    error = ySetPoint - y;
//
//    // calculate integral and prevent windup
//    integral += error;
//
//    int32_t iLimit = iMax * coeffKi;
//    if (integral > iLimit)
//        integral = iLimit;
//    if (integral < -iLimit)
//        integral = -iLimit;
//
//    // calculate differential
//    diff = error - errorLast;
//    errorLast = error;
//
//    // calculate plant input, saturating 10 bit output if needed
//    u = (coeffKp * error) + (coeffKi * integral) + (coeffKd * diff);
//    u /= coeffK;
//    if (u > 1023) u = 1023;
//    if (u < -1023) u = -1023;
//
//    // set direction and speed based on mode
//    setMotor0_Direction(u >= 0);
//    if (abs(error) > deadBand)
//    {
//        PWM0_DutyCycle = abs(u)+ coeffKo;
//    }
//    else
//    {
//        PWM0_DutyCycle = 0;
//    }
//
//
//    TIMER2_ICR_R = TIMER_ICR_TATOCINT;
//}

void PWMSetDeadBand(void)
{
    // Set the dead band times for all three PWM generators.
    PWMDeadBandEnable(PWM0_BASE, PWM_GEN_0, DEAD_TIME, DEAD_TIME);
    PWMDeadBandEnable(PWM0_BASE, PWM_GEN_1, DEAD_TIME, DEAD_TIME);
    PWMDeadBandEnable(PWM0_BASE, PWM_GEN_2, DEAD_TIME, DEAD_TIME);
}

void PWMClearDeadBand(void) //Turn off Dead-Band Generator
{
    PWMDeadBandDisable(PWM0_BASE, PWM_GEN_0);
    PWMDeadBandDisable(PWM0_BASE, PWM_GEN_1);
    PWMDeadBandDisable(PWM0_BASE, PWM_GEN_2);
}

void setMotorPwm(void)
{
    PWM0_0_CMPA_R = PWM0_DutyCycle;
    PWM0_0_CMPB_R = PWM0_DutyCycle;
    PWM1_1_CMPA_R = PWM0_DutyCycle;
    PWM1_1_CMPA_R = PWM0_DutyCycle;
    PWM1_2_CMPA_R = PWM0_DutyCycle;
    PWM1_2_CMPA_R = PWM0_DutyCycle;

    PWMSyncUpdate(PWM0_BASE, PWM_GEN_0_BIT | PWM_GEN_1_BIT | PWM_GEN_2_BIT);
}

void PWM0_ISR(void)
{
    PWMGenIntClear(PWM0_BASE, PWM_GEN_0, PWM_INT_CNT_ZERO);
    PWMGenIntClear(PWM0_BASE, PWM_GEN_0, PWM_INT_CNT_ZERO); //Incase something else triggers it again?
    setMotorPwm();
}


//Enables the respective PWMs for commutation. This mask comes from the hall sensor update
void PWM0_OutputUpdate(uint8_t Enable)
{
    //
    // If the motor drive is in a faulted state, don't do anything else.
    //
//    if(MainIsFaulted())
//    {
//        return;
//    }

    // Disable ADC interrupts that reference the PWM output/invert states.
    IntDisable(INT_ADC0SS0);

    // Disable the non-selected PWM outputs.
    PWMOutputState(PWM0_BASE, Enable ^ (PWM_OUT_0_BIT | PWM_OUT_1_BIT | PWM_OUT_2_BIT | PWM_OUT_3_BIT | PWM_OUT_4_BIT | PWM_OUT_5_BIT), false);
    //
    // Enable the selected PWM high phase outputs.
    PWMOutputState(PWM0_BASE, Enable, true);

    // Reenable the ADC interrupts that reference the PWM output/invert states.
    IntEnable(INT_ADC0SS0);
}


void PWMOutputOff(void)
{
    // Disable all six PWM outputs.
    PWMOutputState(PWM0_BASE, (PWM_OUT_0_BIT | PWM_OUT_1_BIT | PWM_OUT_2_BIT | PWM_OUT_3_BIT | PWM_OUT_4_BIT | PWM_OUT_5_BIT), false);

    // Set the PWM duty cycles to 50%.
    PWM0_DutyCycle = PWM_PERIOD/2;
    //
    // Set the PWM period so that the ADC runs at 1 KHz.
    //
//    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, PWM_PERIOD);
//    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_1, PWM_PERIOD);
//    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, PWM_PERIOD);

    //
    // Disable Deadband and update the PWM duty cycles.
    //
    PWMClearDeadBand();
    setMotorPwm();
}

void setMotor0_Direction(bool dir)
{
    Motor0_Direction = dir;
}

void initPWM(void)
{

    SysCtlPWMClockSet (SYSCTL_PWMDIV_1); //Use same frequency as Sys_Clk (40Mhz)

    // Enable the peripherals used by this program.
    SysCtlPeripheralEnable (SYSCTL_PERIPH_GPIOB);
    SysCtlPeripheralEnable (SYSCTL_PERIPH_GPIOE);
    SysCtlPeripheralEnable (SYSCTL_PERIPH_PWM0); /// PWM Module 0 for Motor 1

    GPIOPinConfigure (GPIO_PB6_M0PWM0);
    GPIOPinConfigure (GPIO_PB7_M0PWM1);
    GPIOPinConfigure (GPIO_PB4_M0PWM2);
    GPIOPinConfigure (GPIO_PB5_M0PWM3);
    GPIOPinConfigure (GPIO_PE4_M0PWM4);
    GPIOPinConfigure (GPIO_PE5_M0PWM5);

    //Configure PB6,PB7,PB4, PE4, PE5 Pins as PWM
    GPIOPinTypePWM(GPIO_PORTB_BASE, GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_4 | GPIO_PIN_5); //PWM 0,1,2,3 (In order)
    GPIOPinTypePWM(GPIO_PORTE_BASE, GPIO_PIN_4 | GPIO_PIN_5); //PWM 4 and 5

    // Configure the three PWM generators for up/down counting mode,
    // synchronous updates, and to stop at zero on debug events.
    PWMGenConfigure(PWM0_BASE, PWM_GEN_0, (PWM_GEN_MODE_UP_DOWN | PWM_GEN_MODE_SYNC | PWM_GEN_MODE_DBG_STOP));
    PWMGenConfigure(PWM0_BASE, PWM_GEN_1, (PWM_GEN_MODE_UP_DOWN | PWM_GEN_MODE_SYNC | PWM_GEN_MODE_DBG_STOP));
    PWMGenConfigure(PWM0_BASE, PWM_GEN_2, (PWM_GEN_MODE_UP_DOWN | PWM_GEN_MODE_SYNC | PWM_GEN_MODE_DBG_STOP));

    // Set the initial duty cycles to 50%.
    PWM0_DutyCycle = 1;

    PWMClearDeadBand();
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, PWM_PERIOD);
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_1, PWM_PERIOD);
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, PWM_PERIOD);
    setMotorPwm();

    //
    // Configure an interrupt on the zero event of the first generator, and an
    // ADC trigger on the load event of the first generator.
    //
    PWMGenIntClear(PWM0_BASE, PWM_GEN_0, PWM_INT_CNT_ZERO);
    PWMGenIntTrigEnable(PWM0_BASE, PWM_GEN_0, PWM_INT_CNT_ZERO | PWM_TR_CNT_LOAD);
    PWMGenIntTrigEnable(PWM0_BASE, PWM_GEN_1, 0);
    PWMGenIntTrigEnable(PWM0_BASE, PWM_GEN_2, 0);
    IntEnable(INT_PWM0_0);
    IntEnable(INT_PWM0_1);
    IntEnable(INT_PWM0_2);

    // Set all six PWM outputs to go to the inactive state when a fault event
    PWMOutputFault(PWM0_BASE, (PWM_OUT_0_BIT | PWM_OUT_1_BIT | PWM_OUT_2_BIT | PWM_OUT_3_BIT | PWM_OUT_4_BIT | PWM_OUT_5_BIT), true);

    // Disable all six PWM outputs.
    PWMOutputState(PWM0_BASE, (PWM_OUT_0_BIT | PWM_OUT_1_BIT | PWM_OUT_2_BIT | PWM_OUT_3_BIT | PWM_OUT_4_BIT | PWM_OUT_5_BIT),false);
    PWMOutputInvert(PWM0_BASE, (PWM_OUT_0_BIT | PWM_OUT_1_BIT | PWM_OUT_2_BIT | PWM_OUT_3_BIT | PWM_OUT_4_BIT | PWM_OUT_5_BIT),false);

    // Enable the PWM generators.
    PWMGenEnable(PWM0_BASE, PWM_GEN_0);
    PWMGenEnable(PWM0_BASE, PWM_GEN_1);
    PWMGenEnable(PWM0_BASE, PWM_GEN_2);

    // Synchronize the time base of the generators.
    PWMSyncTimeBase(PWM0_BASE, PWM_GEN_0_BIT | PWM_GEN_1_BIT | PWM_GEN_2_BIT);
}







