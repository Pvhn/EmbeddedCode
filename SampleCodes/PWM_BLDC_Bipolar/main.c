#include "control.h"


#define MINIMUM_PWM_DUTY_CYCLE 20

// Use AIN3 for ADC, PIN PE0



uint8_t Halls, Hall_Int_Status, Prev_Hall_Sequence;
uint8_t Duty = 0;

bool Hall_Stopped = true;
bool Hall_State_Unknown = true;

unsigned int Desired_PWM_Duty_Cycle;

#ifdef DIRECTION_CCW
uint8_t Hall_Sequence[] = { 0, //Fault Condition (All off)
        PWM4_CH|PWM3_BL, // Hall 001
        PWM2_BH|PWM1_AL, // Hall 010
        PWM4_CH|PWM1_AL, // Hall 011
        PWM0_AH|PWM5_CL, // Hall 100
        PWM0_AH|PWM3_BL, // Hall 101
        PWM2_BH|PWM5_CL, // Hall 110
        0 // Fault Condition (All on)
        //Read as Hall 1,2,3 (PA2,3,4) or Hall A,B,C
};
#endif

#ifdef DIRECTION_CW //Ignore this for now
uint8_t Hall_Sequence[] = { 0, //Fault Condition (All off)
        PWM2_BH | PWM5_CL, // Hall 001
        PWM0_AH | PWM3_BL, // Hall 010
        PWM0_AH | PWM5_CL, // Hall 011
        PWM4_CH | PWM1_AL, // Hall 100
        PWM2_BH | PWM1_AL, // Hall 101
        PWM4_CH | PWM3_BL, // Hall 110
        0 // Fault Condition (All on)
        };
#endif

volatile int qeiPosition;

// Function definition
void PWM_update(unsigned char Next_Hall_Sequence);
void Start_Motor(void);
void Stop_Motor(void);
void Start_ADC_Conversion(void);

void waitNanosecond(void)
{
    __asm(" NOP");
    __asm(" NOP");
    __asm(" NOP");
    __asm(" NOP");
    __asm(" NOP");
}

void PWM_Update(uint8_t Nxt_Hall_Sequence)
{

    switch (Nxt_Hall_Sequence)
    {

    case PWM0_AH | PWM5_CL:
        PWMOutputState(PWM0_BASE, PWM2_BH | PWM3_BL, false);
        PWMPulseWidthSet(PWM0_BASE,PWM_OUT_0, (PWM_PERIOD *Duty)/100);
        PWMPulseWidthSet(PWM0_BASE,PWM_OUT_4, (PWM_PERIOD *(100-Duty))/100);
        PWMOutputState(PWM0_BASE, PWM0_AH | PWM1_AL | PWM4_CH | PWM5_CL, true);
        break;
    case PWM2_BH | PWM1_AL:
        PWMOutputState(PWM0_BASE, PWM4_CH | PWM5_CL, false);
        PWMPulseWidthSet(PWM0_BASE,PWM_OUT_2, (PWM_PERIOD *Duty)/100);
        PWMPulseWidthSet(PWM0_BASE,PWM_OUT_0, PWM_PERIOD);
        PWMOutputState(PWM0_BASE, PWM0_AH | PWM1_AL | PWM2_BH | PWM3_BL, true);
        break;
    case PWM2_BH | PWM5_CL:
        PWMOutputState(PWM0_BASE, PWM0_AH | PWM1_AL, false);
        PWMPulseWidthSet(PWM0_BASE,PWM_OUT_2, (PWM_PERIOD *Duty)/100);
        PWMPulseWidthSet(PWM0_BASE,PWM_OUT_4, PWM_PERIOD);
        PWMOutputState(PWM0_BASE, PWM2_BH | PWM3_BL | PWM4_CH | PWM5_CL, true);
        break;
    case PWM4_CH | PWM3_BL:
        PWMOutputState(PWM0_BASE, PWM0_AH | PWM1_AL, false);
        PWMPulseWidthSet(PWM0_BASE,PWM_OUT_4, (PWM_PERIOD *Duty)/100);
        PWMPulseWidthSet(PWM0_BASE,PWM_OUT_2, PWM_PERIOD);
        PWMOutputState(PWM0_BASE, PWM2_BH | PWM3_BL | PWM4_CH | PWM5_CL, true);
        break;
    case PWM0_AH | PWM3_BL:
        PWMOutputState(PWM0_BASE, PWM4_CH | PWM5_CL, false);
        PWMPulseWidthSet(PWM0_BASE,PWM_OUT_0, (PWM_PERIOD *Duty)/100);
        PWMPulseWidthSet(PWM0_BASE,PWM_OUT_2, PWM_PERIOD);
        PWMOutputState(PWM0_BASE, PWM0_AH | PWM1_AL | PWM2_BH | PWM3_BL, true);
        break;
    case PWM4_CH | PWM1_AL:
        PWMOutputState(PWM0_BASE, PWM2_BH | PWM3_BL, false);
        PWMPulseWidthSet(PWM0_BASE,PWM_OUT_4, (PWM_PERIOD *Duty)/100);
        PWMPulseWidthSet(PWM0_BASE,PWM_OUT_0, PWM_PERIOD);
        PWMOutputState(PWM0_BASE, PWM0_AH | PWM1_AL | PWM4_CH | PWM5_CL, true);
        break;
    default:
        PWMOutputState(PWM0_BASE, PWM0_AH | PWM2_BH | PWM3_BL | PWM5_CL | PWM4_CH | PWM1_AL, false);
        break;
    }
    PWMSyncTimeBase(PWM0_BASE, PWM0_AH | PWM2_BH | PWM3_BL | PWM5_CL | PWM4_CH | PWM1_AL);
//    PWMSyncUpdate(PWM0_BASE, PWM_GEN_0_BIT | PWM_GEN_1_BIT | PWM_GEN_2_BIT |PWM_GEN_3_BIT );
}

void PORTAISR(void) //Hall Status ISR
{
    //GPIOIntDisable(GPIO_PORTA_BASE, GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4);
    GPIOIntClear(GPIO_PORTA_BASE, GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4); //Clear Flag
    Hall_Int_Status = GPIOIntStatus(GPIO_PORTA_BASE, false); //Returns Interrupt Status
    Halls = GPIOPinRead(GPIO_PORTA_BASE, GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4); //Read Hall inputs
    Halls = (Halls >> 2) & 7; //Shift value so that it's value is 0-7.
    PWM_Update(Hall_Sequence[Halls]);
}

//Sets PWM Duty Cycle
void setPWM_Duty(uint8_t Duty)
{
    //Duty = 100-Duty;
    if (Duty <= 2)
    {
        PWMPulseWidthSet(PWM0_BASE, PWM_OUT_0, 1);
        PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, 1);
        PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, 1);
        PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, 1);
        PWMPulseWidthSet(PWM0_BASE, PWM_OUT_3, 1);
        PWMPulseWidthSet(PWM0_BASE, PWM_OUT_5, 1);
    }
    else
    {
        //High Side vary only
        PWMPulseWidthSet(PWM0_BASE, PWM_OUT_0, (PWM_PERIOD * Duty) / 100);
        PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, (PWM_PERIOD * Duty) / 100);
        PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, (PWM_PERIOD * Duty) / 100);
    }
}


uint16_t position = 0;
uint32_t fault_status = 0;
void Start_Motor(void)
{
    setPWM_Duty(0);
    Halls = GPIOPinRead(GPIO_PORTA_BASE, GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4); //Read Hall inputs
    Halls = (Halls >> 2) & 7; //Shift value so that it's value is 0-7.
    Prev_Hall_Sequence = Hall_Sequence[Halls];
    PWM_Update(Prev_Hall_Sequence);
}

void main(void)
  {

    uint32_t raw_position = 0;
    initHW();
    initTimer();
    initPWM();
    initADC();
    initQEI();

    Start_Motor();

    //Do nothing
    while (1)
    {
        ADCProcessorTrigger(ADC0_BASE, 0);
        ADCProcessorTrigger(ADC0_BASE, 2);
        ADCProcessorTrigger(ADC0_BASE, 1);
        setPWM_Duty((Motor_Data[2]*100)/4096);
//        raw_position = (QEIPositionGet(QEI0_BASE)*360);
//        position = raw_position/6400;
        position = QEIPositionGet(QEI0_BASE);
        fault_status =PWM0_0_FLTSTAT1_R;
    }
}
