
#include "control.h"
#include "main.h"

#define COMP_LIMIT 3195 //Comparator limit for high band. See README for calculation
#define MAX_TICKS 6399 //Max encoder ticks. See encoder data sheet


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

uint8_t Hall_SequenceCW[] = { 0, //Fault Condition (All off)
        PWM2_BH | PWM5_CL, // Hall 001
        PWM0_AH | PWM3_BL, // Hall 010
        PWM0_AH | PWM5_CL, // Hall 011
        PWM4_CH | PWM1_AL, // Hall 100
        PWM2_BH | PWM1_AL, // Hall 101
        PWM4_CH | PWM3_BL, // Hall 110
        0 // Fault Condition (All on)
        };

uint8_t Halls = 0;
uint8_t Motor0_Direction = 0; //0 = CW, 1 = CCW
uint8_t Motor0_Enable = 1, Motor0_status = 0;
uint16_t PWM0_Duty = 0;

bool motor1_update_flag = false;
bool motor1_position_update_flag = false;
bool battery_update_flag = false;

uint16_t ADC0SS0_Raw[3] = {0};
uint16_t ADC0SS1_Raw = 0;
uint16_t ADC0SS2_Raw = 0;

void initFPU() //Used for float calculations
{
    FPUEnable();
    FPULazyStackingEnable();

}

void PWM0_OutputUpdate(uint8_t Enable)
{
    // Disable the non-selected PWM outputs.
    PWMOutputState(PWM0_BASE, Enable ^ (PWM_OUT_0_BIT | PWM_OUT_1_BIT | PWM_OUT_2_BIT | PWM_OUT_3_BIT | PWM_OUT_4_BIT | PWM_OUT_5_BIT), false);
    //
    // Enable the selected PWM high phase outputs.
    PWMOutputState(PWM0_BASE, Enable, true);
    PWMSyncTimeBase(PWM0_BASE, PWM0_AH | PWM2_BH | PWM3_BL | PWM5_CL | PWM4_CH | PWM1_AL);
}

void Stop_Motor0(void)
{
    IntDisable(INT_GPIOA);
    PWMOutputState(PWM0_BASE, (PWM_OUT_0_BIT | PWM_OUT_1_BIT | PWM_OUT_2_BIT | PWM_OUT_3_BIT | PWM_OUT_4_BIT | PWM_OUT_5_BIT), false);
}

void setPWM0_Duty(uint32_t Duty)
{
    uint16_t period = 0;
    if(Duty >= MAX_PWM)
    {
        Duty = MAX_PWM; //Set maximum
    }
    else if(Duty < MIN_PWM)
    {
       Duty = MIN_PWM; //Minimum
    }
    period = PWM_LOAD - Duty;
    HWREG(PWM0_BASE + PWM_O_0_CMPA) = period;
    HWREG(PWM0_BASE + PWM_O_0_CMPB) = period;
    HWREG(PWM0_BASE + PWM_O_1_CMPA) = period;
    HWREG(PWM0_BASE + PWM_O_1_CMPB) = period;
    HWREG(PWM0_BASE + PWM_O_2_CMPA) = period;
    HWREG(PWM0_BASE + PWM_O_2_CMPB) = period;
}

void Start_Motor0(void)
{
    IntEnable(INT_GPIOA);
    Halls = GPIOPinRead(GPIO_PORTA_BASE, GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4)>>2; //Read Hall inputs
    Motor0_Direction = GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_2)>>2; //Read Motor Direction
    if(Motor0_Direction)
    {
        PWM0_OutputUpdate(Hall_SequenceCW[Halls]);
    }
    else
    {
        PWM0_OutputUpdate(Hall_SequenceCCW[Halls]);
    }
}

void init_Motor0(void) //Read Initial Positions
{
    Halls = GPIOPinRead(GPIO_PORTA_BASE, GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4)>>2; //Read Hall inputs
    //Motor0_Enable = GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_3)>>2;
    Motor0_Direction = GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_2)>>2; //Read Motor Direction
}

void PORTA_ISR(void) //Hall Status ISR
{
    //GPIOIntDisable(GPIO_PORTA_BASE, GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4);
    GPIOIntClear(GPIO_PORTA_BASE, GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 );
    Halls = GPIOPinRead(GPIO_PORTA_BASE, GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4)>>2; //Read Motor Direction
    if(Motor0_Direction)
    {
        PWM0_OutputUpdate(Hall_SequenceCW[Halls]);
    }
    else
    {
        PWM0_OutputUpdate(Hall_SequenceCCW[Halls]);
    }
}

void PORTF_ISR(void) //Motor Enable ISR
{
    GPIOIntClear(GPIO_PORTF_BASE, GPIO_PIN_2 | GPIO_PIN_3);
    Motor0_Enable = GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_3)>>2; // Read Enable
    Motor0_Direction = GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_2)>>2; //Read Hall inputs
//    if(Motor0_Enable)
//    {
//        if(Motor0_status == STOPPED)
//        {
//          Start_Motor0();
//          Motor0_status = RUNNING;
//        }
//    }
//    else
//    {
//        Stop_Motor0();
//        Motor0_status = STOPPED;
//    }
}

void initHW()
{
    //Set the clock
    SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ); //40 Mhz
    SysCtlPeripheralEnable (SYSCTL_PERIPH_GPIOA); //Enable Port A
    SysCtlPeripheralEnable (SYSCTL_PERIPH_GPIOF);
    SysCtlDelay(10); // Time for the clock configuration to set

    // Configure GPIO Type
    GPIOPinTypeGPIOInput(GPIO_PORTA_BASE, GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4); //Hall Sensor Pins
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_2 | GPIO_PIN_3); //LEDs (On TIVA board)

    //Configure PortA Interrupts
    IntEnable(INT_GPIOA);
    GPIOIntTypeSet(GPIO_PORTA_BASE, GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4, GPIO_BOTH_EDGES); //Detect rise and fall of hall sensors
    GPIOIntEnable(GPIO_PORTA_BASE, GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4); // Enable interrupt

    IntEnable(INT_GPIOF);
    GPIOIntTypeSet(GPIO_PORTF_BASE, GPIO_PIN_2 | GPIO_PIN_3, GPIO_BOTH_EDGES); //Detect high or low
    GPIOIntEnable(GPIO_PORTF_BASE, GPIO_PIN_2 | GPIO_PIN_3); // Enable interrupt
}

void PWM0_ISR(void)
{
    PWMGenIntClear(PWM0_BASE, PWM_GEN_0, PWM_INT_CNT_LOAD);
    //PWMGenFaultClear(PWM0_BASE,PWM_GEN_0, PWM_FAULT_GROUP_1, PWM_FAULT_DCMP0);
    PWMSyncTimeBase(PWM0_BASE, PWM_GEN_0_BIT | PWM_GEN_1_BIT | PWM_GEN_2_BIT);
}

void initPWM()
{
    //Configure PWM Clock to match system

    SysCtlPWMClockSet (SYSCTL_PWMDIV_1); //Use same frequency as Sys_Clk (40Mhz)

    // Enable the peripherals used by this program.
    SysCtlPeripheralEnable (SYSCTL_PERIPH_GPIOB);
    SysCtlPeripheralEnable (SYSCTL_PERIPH_GPIOE);
    SysCtlPeripheralEnable (SYSCTL_PERIPH_GPIOD);
    SysCtlPeripheralEnable (SYSCTL_PERIPH_PWM0); /// PWM Module 0 for Motor 1
    //Module 0 for Motor 1

    HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
    HWREG(GPIO_PORTF_BASE + GPIO_O_CR) |= 0x04;
    HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = 0;
    GPIOPinConfigure (GPIO_PB6_M0PWM0);
    GPIOPinConfigure (GPIO_PB7_M0PWM1);
    GPIOPinConfigure (GPIO_PB4_M0PWM2);
    GPIOPinConfigure (GPIO_PB5_M0PWM3);
    GPIOPinConfigure (GPIO_PE4_M0PWM4);
    GPIOPinConfigure (GPIO_PE5_M0PWM5);
    GPIOPinConfigure (GPIO_PD2_M0FAULT0);
    GPIOPinTypeGPIOInput(GPIO_PORTD_BASE, GPIO_PIN_2); //Fault Pin
    HWREG(GPIO_PORTD_BASE + GPIO_O_AFSEL) |= 0x04; //Set PD2 as Fault Pin

    //Configure PB6,PB7,PB4, PE4, PE5 Pins as PWM
    GPIOPinTypePWM(GPIO_PORTB_BASE, GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_4 | GPIO_PIN_5); //PWM 0,1,2,3 (In order)
    GPIOPinTypePWM(GPIO_PORTE_BASE, GPIO_PIN_4 | GPIO_PIN_5); //PWM 4 and 5

    //Configure PWM Options
    PWMGenConfigure(PWM0_BASE, PWM_GEN_0, PWM_GEN_MODE_UP_DOWN | PWM_GEN_MODE_GEN_NO_SYNC);
    PWMGenConfigure(PWM0_BASE, PWM_GEN_1, PWM_GEN_MODE_UP_DOWN | PWM_GEN_MODE_GEN_NO_SYNC);
    PWMGenConfigure(PWM0_BASE, PWM_GEN_2, PWM_GEN_MODE_UP_DOWN | PWM_GEN_MODE_GEN_NO_SYNC);

    //Set the Period (expressed in clock ticks)
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, PWM_PERIOD);
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_1, PWM_PERIOD);
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, PWM_PERIOD);

//    PWMDeadBandEnable(PWM0_BASE, PWM_GEN_0, 2, 2);
//    PWMDeadBandEnable(PWM0_BASE, PWM_GEN_1, 2, 2);
//    PWMDeadBandEnable(PWM0_BASE, PWM_GEN_2, 2, 2);
    //Set PWM duty cycle initially to
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_0, PWM_PERIOD/2);
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, PWM_PERIOD/2);
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, PWM_PERIOD/2);
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_3, PWM_PERIOD/2);
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, PWM_PERIOD/2);
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_5, PWM_PERIOD/2);

    //Configuring PWM Faults
    PWMGenFaultConfigure(PWM0_BASE, PWM_GEN_0, PWM_PERIOD/2, PWM_FAULT0_SENSE_HIGH);
    PWMGenFaultConfigure(PWM0_BASE, PWM_GEN_1, PWM_PERIOD/2, PWM_FAULT0_SENSE_HIGH);
    PWMGenFaultConfigure(PWM0_BASE, PWM_GEN_2, PWM_PERIOD/2, PWM_FAULT0_SENSE_HIGH);
    PWMGenFaultTriggerSet(PWM0_BASE, PWM_GEN_0, PWM_FAULT_GROUP_0, PWM_FAULT_FAULT0);
    PWMGenFaultTriggerSet(PWM0_BASE, PWM_GEN_1, PWM_FAULT_GROUP_0, PWM_FAULT_FAULT0);
    PWMGenFaultTriggerSet(PWM0_BASE, PWM_GEN_2, PWM_FAULT_GROUP_0, PWM_FAULT_FAULT0);

    PWMOutputFault(PWM0_BASE, PWM0_AH | PWM1_AL | PWM2_BH | PWM3_BL | PWM4_CH | PWM5_CL, true); //Suppress if Fault TRUE
    PWMOutputFaultLevel(PWM0_BASE, PWM0_AH | PWM1_AL | PWM2_BH | PWM3_BL | PWM4_CH | PWM5_CL, false); //Pull Low if Fault

    //Set Triggers/Interrupts
    PWMGenIntTrigEnable(PWM0_BASE, PWM_GEN_0, PWM_TR_CNT_ZERO); //Trigger ADC for sampling for each cycle
//    PWMGenIntRegister(PWM0_BASE,PWM_GEN_0, PWM0_ISR);
    //PWMIntEnable(PWM0_BASE, PWM_INT_GEN_0);


    // Enable the PWM generator
    PWMGenEnable(PWM0_BASE, PWM_GEN_0);
    PWMGenEnable(PWM0_BASE, PWM_GEN_1);
    PWMGenEnable(PWM0_BASE, PWM_GEN_2);

} 

void ADC0_Seq0_ISR(void)
{
    HWREG(ADC0_BASE + ADC_O_ISC) = 0x01;
    ADC0SS0_Raw[0] = HWREG(ADC0_BASE + ADC_O_SSFIFO0);
    ADC0SS0_Raw[1] = HWREG(ADC0_BASE + ADC_O_SSFIFO0);
    ADC0SS0_Raw[2] = HWREG(ADC0_BASE + ADC_O_SSFIFO0);
    battery_update_flag = true;
    TimerLoadSet(TIMER0_BASE,TIMER_A, PWM_PERIOD); //Timer is set to trigger at same freq as PWM
    TimerEnable(TIMER0_BASE, TIMER_A);
}

void ADC0_Seq1_ISR(void)
{
    ADC0SS1_Raw = 0;
    int i = 0;
    HWREG(ADC0_BASE + ADC_O_ISC) = 0x02;

    //Since we are not using full sequencer, we can simply average the samples
    for(i = 0; i<4; i++)
    {
        ADC0SS1_Raw += HWREG(ADC0_BASE + ADC_O_SSFIFO1);
    }
    ADC0SS1_Raw = ADC0SS1_Raw/4;
    motor1_update_flag = true; //Set flag for update in main

}

void ADC0_Seq2_ISR(void)
{
    ADC0SS2_Raw = 0;
    int i = 0;
    HWREG(ADC0_BASE + ADC_O_ISC) = 0x04; //Clear Flag

    //Since we are not using full sequencer, we can simply average the samples
    for(i = 0; i<4; i++)
    {
        ADC0SS2_Raw += HWREG(ADC0_BASE + ADC_O_SSFIFO2);
    }
    ADC0SS2_Raw = ADC0SS2_Raw/4;
    motor1_position_update_flag = true; //Set flag for update in main
}

void initADC()
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0); // Enable ADC0 //Used for Sampling
    //SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC1);

    ADCClockConfigSet(ADC0_BASE, ADC_CLOCK_SRC_PLL, ADC_CLOCK_RATE_FULL);

    //Configure GPIO as ADC
    GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_3); //PE3 Motor1 Isense
    GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_2); //PE2 Motor2 Isense
    GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_1); //PE1 VBUS sense (48V)
    GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_0); //PE0 Battery Vsense
    GPIOPinTypeADC(GPIO_PORTD_BASE, GPIO_PIN_3); //PD3 Battery Isense

    //Configure Sequence Sampler for ADC0
    ADCSequenceDisable(ADC0_BASE, 0); //SS0 used for Battery and 48V Data (V and I sense)
    ADCSequenceDisable(ADC0_BASE, 1); //SS1 used for Motor 1 I sense
    ADCSequenceDisable(ADC0_BASE, 2); //SS2 used for Motor 2 I sense

    ADCSequenceConfigure(ADC0_BASE, 0, ADC_TRIGGER_TIMER, 0); //SS0 has priority 0 and is always sampling
    ADCSequenceConfigure(ADC0_BASE, 1, ADC_TRIGGER_PWM0 | ADC_TRIGGER_PWM_MOD0, 1); //SS1 has priority 1
    ADCSequenceConfigure(ADC0_BASE, 2, ADC_TRIGGER_TIMER, 2); //SS2 has priority 2

    //Configure SS0
    //===============================================================================
    ADCSequenceStepConfigure(ADC0_BASE, 0, 0, BAT_V_SENSE);
    ADCSequenceStepConfigure(ADC0_BASE, 0, 1, BAT_I_SENSE);
    ADCSequenceStepConfigure(ADC0_BASE, 0, 2, VBUS_SENSE | ADC_CTL_IE | ADC_CTL_END);

    IntEnable(INT_ADC0SS0);

    ADCIntEnable(ADC0_BASE, 0); //Enable Interrupt
    ADCSequenceEnable(ADC0_BASE, 0);   //Enable the ADC
    ADCIntClear(ADC0_BASE, 0);     //Clear interrupt to proceed to  data capture

    //Configure SS1
    //===============================================================================
    ADCSequenceStepConfigure(ADC0_BASE, 1, 0, MTR_1_I_SENSE);
    ADCSequenceStepConfigure(ADC0_BASE, 1, 1, MTR_1_I_SENSE);
    ADCSequenceStepConfigure(ADC0_BASE, 1, 2, MTR_1_I_SENSE);
    ADCSequenceStepConfigure(ADC0_BASE, 1, 3, MTR_1_I_SENSE | ADC_CTL_IE | ADC_CTL_END);

    IntEnable(INT_ADC0SS1);

    ADCIntEnable(ADC0_BASE, 1); //Enable Interrupt
    ADCSequenceEnable(ADC0_BASE, 1);   //Enable the ADC
    ADCIntClear(ADC0_BASE,1);     //Clear interrupt to proceed to  data capture

    //Configure SS2
    //===============================================================================
    ADCSequenceStepConfigure(ADC0_BASE, 2, 0, POSITION_POT);
    ADCSequenceStepConfigure(ADC0_BASE, 2, 1, POSITION_POT);
    ADCSequenceStepConfigure(ADC0_BASE, 2, 2, POSITION_POT);
    ADCSequenceStepConfigure(ADC0_BASE, 2, 3, POSITION_POT | ADC_CTL_IE | ADC_CTL_END);

    IntEnable(INT_ADC0SS2);

    ADCIntEnable(ADC0_BASE, 2); //Enable Interrupt
    ADCSequenceEnable(ADC0_BASE, 2);   //Enable the ADC
    ADCIntClear(ADC0_BASE,2);     //Clear interrupt to proceed to  data capture
}

void initTimer()
{
    //Init Timer 0 (Used for ADC Sampling)
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    TimerDisable(TIMER0_BASE, TIMER_A);
    TimerConfigure(TIMER0_BASE, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_ONE_SHOT);
    TimerADCEventSet(TIMER0_BASE, TIMER_ADC_TIMEOUT_A);
    TimerControlTrigger(TIMER0_BASE, TIMER_A, true);
    TimerLoadSet(TIMER0_BASE,TIMER_A, PWM_PERIOD); //Timer is set to trigger at same freq as PWM
    TimerEnable(TIMER0_BASE, TIMER_A);
}

void initComparator()
{
    //Enable Peripherals
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_COMP0);

    //Unlock GPIOF0 for use
    HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
    HWREG(GPIO_PORTF_BASE + GPIO_O_CR) |= 0x01;
    HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = 0;

    //Configure Comparator Pins
    GPIOPinTypeComparator(GPIO_PORTC_BASE, GPIO_PIN_7);
    GPIOPinConfigure(GPIO_PF0_C0O);
    GPIOPinTypeComparatorOutput(GPIO_PORTF_BASE, GPIO_PIN_0);


    ComparatorConfigure(COMP_BASE, 0, COMP_ASRCP_REF | COMP_OUTPUT_INVERT);
    ComparatorRefSet(COMP_BASE, COMP_REF_2_371875V);
}

void initQEI()
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD); // Enable Port D (QEI0)
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC); // Enable Port C (QEI1)
    SysCtlPeripheralEnable(SYSCTL_PERIPH_QEI0); // Enable QEI Module 0
    SysCtlPeripheralEnable(SYSCTL_PERIPH_QEI1); // Enable QEI Module 1

    //Unlock GPIOD7 - Like PF0 its used for NMI - Without this step it doesn't work
     HWREG(GPIO_PORTD_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
     HWREG(GPIO_PORTD_BASE + GPIO_O_CR) |= 0x80;
     HWREG(GPIO_PORTD_BASE + GPIO_O_LOCK) = 0;

     //Configure QEI Pins
     GPIOPinConfigure(GPIO_PD6_PHA0);
     GPIOPinConfigure(GPIO_PD7_PHB0);
     GPIOPinConfigure(GPIO_PC5_PHA1);
     GPIOPinConfigure(GPIO_PC6_PHB1);

     //Set GPIO pins for QEI
     GPIOPinTypeQEI(GPIO_PORTD_BASE, GPIO_PIN_6 |  GPIO_PIN_7); //QEI0
     GPIOPinTypeQEI(GPIO_PORTC_BASE, GPIO_PIN_5 |  GPIO_PIN_6); //QEI1

     //Disable peripheral and interrupt before configuration
     QEIDisable(QEI0_BASE);
     QEIIntDisable(QEI0_BASE,QEI_INTERROR | QEI_INTDIR | QEI_INTTIMER | QEI_INTINDEX);

     QEIDisable(QEI1_BASE);
     QEIIntDisable(QEI1_BASE,QEI_INTERROR | QEI_INTDIR | QEI_INTTIMER | QEI_INTINDEX);

     // Configure quadrature encoder
     QEIConfigure(QEI0_BASE, (QEI_CONFIG_CAPTURE_A_B  | QEI_CONFIG_NO_RESET  | QEI_CONFIG_QUADRATURE | QEI_CONFIG_NO_SWAP), MAX_TICKS);
     QEIConfigure(QEI1_BASE, (QEI_CONFIG_CAPTURE_A_B  | QEI_CONFIG_NO_RESET  | QEI_CONFIG_QUADRATURE | QEI_CONFIG_NO_SWAP), MAX_TICKS);

     // Enable the quadrature encoder.
     QEIEnable(QEI0_BASE);
     QEIEnable(QEI1_BASE);

     //Set initial position to 0
     QEIPositionSet(QEI0_BASE, 0);
     QEIPositionSet(QEI1_BASE, 0);
}



