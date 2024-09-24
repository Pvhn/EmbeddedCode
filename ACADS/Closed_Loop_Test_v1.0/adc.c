/*
 * adc.c
 *
 *  Created on: Mar 31, 2021
 *      Author: peter
 */
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_gpio.h"
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "tm4c123gh6pm.h"
#include "inc/hw_adc.h"
#include <driverlib/sysctl.h>
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"
#include "driverlib/adc.h"
#include "driverlib/interrupt.h"
#include "adc.h"

#define DC_POT          0x00000000  // Input channel 0 PE0 --> OPEN LOOP ONLY. Used for sampling duty from pot
#define VBUS_SENSE      0x00000001  // Input channel 1 PE2
#define MOTOR_2_I_SENSE 0x00000002  // Input channel 2 PE1
#define MOTOR_1_I_SENSE 0x00000003  // Input channel 3 PE0
#define BAT_I_SENSE     0x00000004  // Input channel 4 PD3
#define BAT_V_SENSE     0x00000005  // Input channel 5 PD2

uint32_t ADC0DataRaw[5];

void ADC0_Seq0_ISR(void)
{
    uint16_t temp = 0;
    ADCIntClear(ADC0_BASE,0);
    ADC0DataRaw[0] = HWREG(ADC0_BASE + ADC_O_SSFIFO0);
    ADC0DataRaw[1] = HWREG(ADC0_BASE + ADC_O_SSFIFO0);
    ADC0DataRaw[2] = HWREG(ADC0_BASE + ADC_O_SSFIFO0);
    ADC0DataRaw[3] = HWREG(ADC0_BASE + ADC_O_SSFIFO0);
    ADC0DataRaw[4] = HWREG(ADC0_BASE + ADC_O_SSFIFO0);

    if((HWREG(ADC0_BASE + ADC_O_OSTAT) & ADC_OSTAT_OV0) || (HWREG(ADC0_BASE + ADC_O_USTAT) & ADC_USTAT_UV0) || (!(HWREG(ADC0_BASE + ADC_O_SSFSTAT0) & ADC_SSFSTAT0_EMPTY)))
    {
        // Disable the sequence.
        HWREG(ADC0_BASE + ADC_O_ACTSS) &= ~ADC_ACTSS_ASEN0;
        // Drain the Sequence FIFO.//
        while(!(HWREG(ADC0_BASE + ADC_O_SSFSTAT0) & ADC_SSFSTAT0_EMPTY))
        {
            //
            // Read the next sample.
            //
            temp = HWREG(ADC0_BASE + ADC_O_SSFIFO0);
        }
        // Clear any overflow/underflow conditions that might exist.
        HWREG(ADC0_BASE + ADC_O_OSTAT) = ADC_OSTAT_OV0;
        HWREG(ADC0_BASE + ADC_O_USTAT) = ADC_USTAT_UV0;
        // Renable the sequence and return.
        HWREG(ADC0_BASE + ADC_O_ACTSS) |= ADC_ACTSS_ASEN0;
        return;
    }
}

void initADC(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0); // Enable ADC0 (Used for Data Sampling)

    GPIOPinTypeADC(GPIO_PORTD_BASE, GPIO_PIN_2); //PD2 Battery Vsense (Ch5)
    GPIOPinTypeADC(GPIO_PORTD_BASE, GPIO_PIN_3); //PD3 Battery Isense (Ch4)
    GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_0); //PE0 Motor1 Isense (Ch3)
    GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_1); //PE1 Motor2 Isense (Ch2)
    GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_2); //PE2 48V Vsense (Ch1)
    //SysCtlADCSpeedSet(SYSCTL_ADCSPEED_1MSPS);
    //
    // Disable the ADC sequence and interrupts for safe reconfiguration of
    // the ADC sequences.
    //
    IntDisable(INT_ADC0SS0);
    ADCIntDisable(ADC0_BASE, 0);
    ADCSequenceDisable(ADC0_BASE, 0);

    //
    // Ensure that this sequence is the highest priority sequence
    // (in the event that other ADC sequences are being used
    // elsewhere in the system).
    //
    ADCSequenceConfigure(ADC0_BASE, 0, ADC_TRIGGER_PWM0, 0);
    ADCSequenceConfigure(ADC0_BASE, 3, ADC_TRIGGER_PWM0, 1); //For Open Loop Used to sample the Duty Cycle
    //==============================================================================================================================
    //Configure SS0
    ADCSequenceStepConfigure(ADC0_BASE, 0, 0, BAT_V_SENSE); //PD2 Ch5 = Battery VSense
    ADCSequenceStepConfigure(ADC0_BASE, 0, 1, BAT_I_SENSE); //Configure ADC to read from channel 2 ,trigger the interrupt to end data capture //
    ADCSequenceStepConfigure(ADC0_BASE, 0, 2, VBUS_SENSE);
    ADCSequenceStepConfigure(ADC0_BASE, 0, 3, MOTOR_1_I_SENSE);
    ADCSequenceStepConfigure(ADC0_BASE, 0, 4, MOTOR_2_I_SENSE | ADC_CTL_IE | ADC_CTL_END);

    //Configure Interrupt
    ADCIntRegister(ADC0_BASE, 0, ADC0_Seq0_ISR); //Designate ISR Handler
    ADCIntEnable(ADC0_BASE, 0); //Enable Interrupt
    ADCSequenceEnable(ADC0_BASE, 0);   //Enable SS0
    ADCIntClear(ADC0_BASE, 0);     //Clear interrupt to proceed to  data capture

    //==============================================================================================================================
    ADCSequenceEnable(ADC0_BASE, 0);
    ADCIntEnable(ADC0_BASE, 0);
    IntEnable(INT_ADC0SS0);
}

