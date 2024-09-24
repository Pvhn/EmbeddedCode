/*
 * ================================== Application Notes =========================================
 *
 * ====================== ADC ========================
 * 5 pins in use
 * 3 Sample Sequencers (SSx) in use
 * SS0 is used for sampling battery data
 * SS1 is for PWM fault handling with the comparator
 * SS2 is used for sampling Motor data
 *
 * PD2 -> Battery Vsense (18V-60V)
 * PD3 -> Battery Current Sense
 * PE2 -> Motor 48V Vsense
 * PE1 -> Motor 1 Isense
 * PE0 -> Motor 2 Isense
 *
 * Current Limit = 9A for Motor Driver
 * Gain = 0.2V/A --> Refer to INA253A2 Datasheet
 * Voltage Divider Scaling Factor = 3.3/5 = 0.66
 *
 * Use the equation below to calculate the appropriate Comparator Limit
 * Note that the limit is with respect to a raw ADC value:
 *
 * Comparator Limit = [(Current Limit* GAIN)+ (5/2)]*(4096/5)
 *
 *========================================================
 *
 *======================= PWM ============================
 * 2 Modules in use
 * Each module uses 3 generators (Gen 0-2) Generator 3 is unused
 * PWM0 = Motor 1
 * PWM1 = Motor 2
 *
 *
 */

#include "control.h"

#define COMP_LIMIT 3522

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

void initTimer()
{
    // Enable clocks
    SYSCTL_RCGCTIMER_R |= SYSCTL_RCGCTIMER_R1;
    _delay_cycles(3);

    // Configure TIMER 1 to run for 50s

    TIMER1_CTL_R &= ~TIMER_CTL_TAEN;
    TIMER1_CFG_R = TIMER_CFG_16_BIT;
    TIMER1_TAMR_R = TIMER_TAMR_TAMR_1_SHOT;
    TIMER1_TAILR_R = 5;
    TIMER1_IMR_R = TIMER_IMR_TATOIM;                 // turn-on interrupts
    NVIC_EN0_R |= 1 << (INT_TIMER1A-16);             // turn-on interrupt 37 (TIMER1A)
}

void initPWM()
{
    //Configure PWM Clock to match system

    SysCtlPWMClockSet (SYSCTL_PWMDIV_1); //Use same frequency as Sys_Clk (40Mhz)

    // Enable the peripherals used by this program.
    SysCtlPeripheralEnable (SYSCTL_PERIPH_GPIOB);
    SysCtlPeripheralEnable (SYSCTL_PERIPH_GPIOE);
    SysCtlPeripheralEnable (SYSCTL_PERIPH_PWM0); /// PWM Module 0 for Motor 1

    //Module 0 for Motor 1
    GPIOPinConfigure (GPIO_PB6_M0PWM0);
    GPIOPinConfigure (GPIO_PB7_M0PWM1);
    GPIOPinConfigure (GPIO_PB4_M0PWM2);
    GPIOPinConfigure (GPIO_PB5_M0PWM3);
    GPIOPinConfigure (GPIO_PE4_M0PWM4);
    GPIOPinConfigure (GPIO_PE5_M0PWM5);

    //Configure PB6,PB7,PB4, PE4, PE5 Pins as PWM
    GPIOPinTypePWM(GPIO_PORTB_BASE, GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_4 | GPIO_PIN_5); //PWM 0,1,2,3 (In order)
    GPIOPinTypePWM(GPIO_PORTE_BASE, GPIO_PIN_4 | GPIO_PIN_5); //PWM 4 and 5

    //Configure PWM Options
    //PWM_GEN_0 = PWM0 and PWM1 (PB6 and PB7)
    //PWM_GEN_1 = PWM2 and PWM3 (PB4 and PB5)
    //PWM_GEN_2 = PWM4 and PWM5 (PE4 and PE5)... Refer to TM4C123gh6pm datasheet and TIVA C Driverlib documents
    PWMGenConfigure(PWM0_BASE, PWM_GEN_0, PWM_GEN_MODE_UP_DOWN | PWM_GEN_MODE_GEN_NO_SYNC | PWM_GEN_MODE_FAULT_LATCHED | PWM_GEN_MODE_FAULT_MINPER |PWM_GEN_MODE_FAULT_EXT); //Configure in center-aligned PWM
    PWMGenConfigure(PWM0_BASE, PWM_GEN_1, PWM_GEN_MODE_UP_DOWN | PWM_GEN_MODE_GEN_NO_SYNC | PWM_GEN_MODE_FAULT_LATCHED | PWM_GEN_MODE_FAULT_MINPER |PWM_GEN_MODE_FAULT_EXT);
    PWMGenConfigure(PWM0_BASE, PWM_GEN_2, PWM_GEN_MODE_UP_DOWN | PWM_GEN_MODE_GEN_NO_SYNC | PWM_GEN_MODE_FAULT_LATCHED | PWM_GEN_MODE_FAULT_MINPER |PWM_GEN_MODE_FAULT_EXT);

//    PWMGenFaultConfigure(PWM0_BASE, PWM_GEN_0,5, PWM_FAULT0_SENSE_HIGH);
//    PWMGenFaultConfigure(PWM0_BASE, PWM_GEN_0,5, PWM_FAULT0_SENSE_HIGH);
//    PWMGenFaultConfigure(PWM0_BASE, PWM_GEN_0,5, PWM_FAULT0_SENSE_HIGH);
//    PWMGenFaultTriggerSet(PWM0_BASE, PWM_GEN_0, PWM_FAULT_GROUP_1,  PWM_FAULT_DCMP0);
//    PWMGenFaultTriggerSet(PWM0_BASE, PWM_GEN_1, PWM_FAULT_GROUP_1,  PWM_FAULT_DCMP0);
//    PWMGenFaultTriggerSet(PWM0_BASE, PWM_GEN_2, PWM_FAULT_GROUP_1, PWM_FAULT_DCMP0);
//    PWMOutputFault(PWM0_BASE, PWM0_AH | PWM2_BH | PWM3_BL | PWM5_CL | PWM4_CH | PWM1_AL, true); //Shutoff PWM if fault occurs
//    PWMOutputFaultLevel(PWM0_BASE, PWM0_AH | PWM2_BH | PWM3_BL | PWM5_CL | PWM4_CH | PWM1_AL, false);

    //Set the Period (expressed in clock ticks)
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, PWM_PERIOD);
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_1, PWM_PERIOD);
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, PWM_PERIOD);

    PWMDeadBandEnable(PWM0_BASE, PWM_GEN_0, 5,5); //120ns dead-band delay
    PWMDeadBandEnable(PWM0_BASE, PWM_GEN_1, 5,5);
    PWMDeadBandEnable(PWM0_BASE, PWM_GEN_2, 5,5);


    //Set PWM duty cycle initially to 0
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_0, 0);
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, 0);
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, 0);
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_3, 0);
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, 0);
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_5, 0);

    PWMIntEnable(PWM0_BASE, PWM_INT_GEN_0 | PWM_INT_GEN_1 | PWM_INT_GEN_2 );
    PWMGenIntRegister(PWM0_BASE, PWM_GEN_0, PWM0_ISR);
    // Enable the PWM generator
    PWMGenEnable(PWM0_BASE, PWM_GEN_0);
    PWMGenEnable(PWM0_BASE, PWM_GEN_1);
    PWMGenEnable(PWM0_BASE, PWM_GEN_2);
} 

void PWM0_ISR(void)
{

}

void initADC()
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0); // Enable ADC0 (Used for Data Sampling)
//    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC1); // Enable ADC1 (For Fault Checking)

    //Configure GPIO as ADC
    GPIOPinTypeADC(GPIO_PORTD_BASE, GPIO_PIN_2); //PD2 Battery Vsense (Ch5)
    GPIOPinTypeADC(GPIO_PORTD_BASE, GPIO_PIN_3); //PD3 Battery Isense (Ch4)
    GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_0); //PE0 Motor1 Isense (Ch3)
    GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_1); //PE1 Motor2 Isense (Ch2)
    GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_2); //PE2 48V Vsense (Ch1)

    //Configure Sequence Sampler for ADC0
    ADCSequenceDisable(ADC0_BASE, 0); //It is always a good practice to disable ADC prior
    ADCSequenceDisable(ADC0_BASE, 1); //It is always a good practice to disable ADC prior
    ADCSequenceDisable(ADC0_BASE, 2); //It is always a good practice to disable ADC prior

    ADCSequenceConfigure(ADC0_BASE, 0, ADC_TRIGGER_PROCESSOR, 1); //SS0 has priority 1
    ADCSequenceConfigure(ADC0_BASE, 1, ADC_TRIGGER_PROCESSOR, 0); //SS1 (Fault Checking) has highest priority
    ADCSequenceConfigure(ADC0_BASE, 2, ADC_TRIGGER_PROCESSOR, 2); //

    //==============================================================================================================================
    //Configure SS0
    ADCSequenceStepConfigure(ADC0_BASE, 0, 0, ADC_CTL_CH5); //PD2 Ch5 = Battery VSense
    ADCSequenceStepConfigure(ADC0_BASE, 0, 1, ADC_CTL_CH4 | ADC_CTL_IE | ADC_CTL_END); //Configure ADC to read from channel 2 ,trigger the interrupt to end data capture //

    //Configure Interrupt
    ADCIntRegister(ADC0_BASE, 0, ADC0_Seq0_ISR); //Designate ISR Handler
    ADCIntEnable(ADC0_BASE, 0); //Enable Interrupt
    ADCSequenceEnable(ADC0_BASE, 0);   //Enable SS0
    ADCIntClear(ADC0_BASE, 0);     //Clear interrupt to proceed to  data capture

    //=============================================================================================================================
    //Configure SS1
    ADCSequenceStepConfigure(ADC0_BASE, 1, 0, ADC_CTL_CH3 | ADC_CTL_CMP0); //Direct Conversion to Comparator 0
    ADCSequenceStepConfigure(ADC0_BASE, 1, 1, ADC_CTL_CH3 | ADC_CTL_CMP0);
    ADCSequenceStepConfigure(ADC0_BASE, 1, 2, ADC_CTL_CH2 | ADC_CTL_CMP1);
    ADCSequenceStepConfigure(ADC0_BASE, 1, 3, ADC_CTL_CH2 | ADC_CTL_CMP1 |ADC_CTL_END); //Direct Conversion to Comparator 1

    //Configure Comparator
    ADCComparatorConfigure(ADC0_BASE, 0, ADC_COMP_TRIG_HIGH_ALWAYS | ADC_COMP_INT_HIGH_HALWAYS); //Trigger PWM Fault when comparison is high
    ADCComparatorConfigure(ADC0_BASE, 1, ADC_COMP_TRIG_HIGH_ALWAYS | ADC_COMP_INT_HIGH_HALWAYS);
    ADCComparatorRegionSet(ADC0_BASE, 0, COMP_LIMIT, COMP_LIMIT); //Set the limit for comparator 0 (Motor 1 Fault)
    ADCComparatorRegionSet(ADC0_BASE, 1, COMP_LIMIT, COMP_LIMIT); //Set the limit for comparator 1 (Motor 2 Fault)
    ADCSequenceEnable(ADC0_BASE, 1);   //Enable SS1

    //==============================================================================================================================
    //Configure SS2
    ADCSequenceStepConfigure(ADC0_BASE, 2, 0, ADC_CTL_CH3); //PE0 Ch3 = Motor1 Isense
    ADCSequenceStepConfigure(ADC0_BASE, 2, 1, ADC_CTL_CH2); //PE1 CH2 = Motor2 Isense
    ADCSequenceStepConfigure(ADC0_BASE, 2, 2, ADC_CTL_CH1 | ADC_CTL_IE | ADC_CTL_END); //PE2 Ch1 = 48V Vsense

    ADCIntRegister(ADC0_BASE, 2, ADC0_Seq2_ISR); //Designate ISR Handler
    ADCIntEnable(ADC0_BASE, 2); //Enable Interrupt
    ADCSequenceEnable(ADC0_BASE, 2);   //Enable SS2
    ADCIntClear(ADC0_BASE, 2);     //Clear interrupt to proceed to  data capture
}

void TIMER1_ISR(void)
{

    TIMER1_CTL_R &= ~TIMER_CTL_TAEN;
    PWMFaultIntClearExt(PWM0_BASE, PWM_INT_FAULT0 | PWM_INT_FAULT1 | PWM_INT_FAULT2 | PWM_INT_FAULT3);

    PWMGenFaultClear(PWM0_BASE, PWM_GEN_0, PWM_FAULT_GROUP_1, PWM_FAULT_DCMP0);
    PWMGenFaultClear(PWM0_BASE, PWM_GEN_1, PWM_FAULT_GROUP_1, PWM_FAULT_DCMP0);
    PWMGenFaultClear(PWM0_BASE, PWM_GEN_2, PWM_FAULT_GROUP_1, PWM_FAULT_DCMP0);
    PWMGenFaultClear(PWM0_BASE, PWM_GEN_0, PWM_FAULT_GROUP_1, PWM_FAULT_DCMP1);
    PWMGenFaultClear(PWM0_BASE, PWM_GEN_1, PWM_FAULT_GROUP_1, PWM_FAULT_DCMP1);
    PWMGenFaultClear(PWM0_BASE, PWM_GEN_2, PWM_FAULT_GROUP_1, PWM_FAULT_DCMP1);
    TIMER1_ICR_R = TIMER_ICR_TATOCINT;
}

void ADC0_Seq0_ISR(void)
{
    ADCIntClear(ADC0_BASE,0); //Clear Flag
    ADCSequenceDataGet(ADC0_BASE, 0, Battery_Data); //Copy FIFO to Buffer
}


void ADC0_Seq2_ISR(void)
{
    ADCIntClear(ADC0_BASE,2); //Clear Flag
    ADCSequenceDataGet(ADC0_BASE, 2, Motor_Data); //Copy FIFO to Buffer
}

void initQEI()
{
    FPUEnable();
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD); // Enable Port D (QEI)
    SysCtlPeripheralEnable(SYSCTL_PERIPH_QEI0); // Enable QEI Module 0

    //Unlock GPIOD7 - Like PF0 its used for NMI - Without this step it doesn't work
     HWREG(GPIO_PORTD_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
     HWREG(GPIO_PORTD_BASE + GPIO_O_CR) |= 0x80;
     HWREG(GPIO_PORTD_BASE + GPIO_O_LOCK) = 0;

     //
     GPIOPinConfigure(GPIO_PD6_PHA0);
     GPIOPinConfigure(GPIO_PD7_PHB0);

     //Set GPIO pins for QEI. PhA0 -> PD6, PhB0 ->PD7.
     GPIOPinTypeQEI(GPIO_PORTD_BASE, GPIO_PIN_6 |  GPIO_PIN_7);

     //Disable peripheral and interrupt before configuration
     QEIDisable(QEI0_BASE);
     QEIIntDisable(QEI0_BASE,QEI_INTERROR | QEI_INTDIR | QEI_INTTIMER | QEI_INTINDEX);

     // Configure quadrature encoder, use an arbitrary top limit of 1000
     QEIConfigure(QEI0_BASE, (QEI_CONFIG_CAPTURE_A_B  | QEI_CONFIG_NO_RESET  | QEI_CONFIG_QUADRATURE | QEI_CONFIG_NO_SWAP), 6400);

     // Enable the quadrature encoder.
     QEIEnable(QEI0_BASE);

     //Set position to a middle value so we can see if things are working
     QEIPositionSet(QEI0_BASE, 0);
}


