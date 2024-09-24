
#include <stdlib.h>
#include "control.h"
#include "main.h"

#define VBUS 48
#define VBAT 60
#define VSUP 5 //Supply Voltage of INA253
#define GAIN 0.2 //Gain of INA253 Current Sense Amplifier
#define ENC_RES 0.05625

#define CAPTURE_SIZE 100

uint8_t dataRead = 0;
bool update_stop_flag = false;
int8_t return_vals[23];

int8_t motor_position[2];

uint8_t avg_input_power = 22;
uint8_t power_generated = 33;

uint16_t raw_encoder1 = 0;
uint16_t raw_encoder2 = 0;

float motor1_I = 0.0, motor2_I = 0.0;
float battery_V = 0.0, battery_I = 0.0, VBUS_V = 0.0;
float avg_output_power = 0.0;
float raw_motor1_pos = 0.0, raw_motor2_pos = 0.0;

// plant output (y) varies from 0 to 1023 (10 bit range), so place in center
int32_t ySetPoint = 512;

// pid calculation of u
int32_t coeffKp = 560;
int32_t coeffKi = 3;
int32_t coeffKd = 10;
int32_t coeffKo = 0; //Ofsset
int32_t coeffK = 100; // denominator used to scale Kp, Ki, and Kd
int32_t integral = 0;
int32_t iMax = 100; //Max value of integral term
int32_t diff;
int32_t error;
int32_t u = 0;
int32_t deadBand = 8;   //If error is less than deadband
int32_t y = 0;
uint32_t yDiv = 1;
uint32_t load = MAX_PWM_LOAD;

// default capture at 1ms rate
int16_t Tcap = 1;
int8_t capturePhase = 0;
// size buffers for plant input and output capture
int8_t captureBufferIndex = 0;
int16_t captureBufferU[CAPTURE_SIZE] = {0};
int16_t captureBufferY[CAPTURE_SIZE] = {0};


void initPID()
{
    //Init Timer 1 (Used for PI Control)
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1); // Enable Timer1
    TimerConfigure(TIMER1_BASE, TIMER_CFG_PERIODIC);
    TimerLoadSet(TIMER1_BASE, TIMER_A, 40000); //Set for 1Khz Interrupt
    TimerEnable(TIMER1_BASE, TIMER_A);

    TimerIntEnable(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
    IntEnable(INT_TIMER1A);
}

void PID0_ISR(void)
{
    static int32_t errorLast = 0;

    y = HWREG(QEI0_BASE + QEI_O_POS);
    // determine error (set point - process value, SP-PV)
    error = ySetPoint - y;

    // calculate integral and prevent windup
    integral += error;

    int32_t iLimit = iMax * coeffKi;
    if (integral > iLimit)
        integral = iLimit;
    if (integral < -iLimit)
        integral = -iLimit;

    // calculate differential
    diff = error - errorLast;
    errorLast = error;

    // calculate plant input, saturating 10 bit output if needed
    u = (coeffKp * error) + (coeffKi * integral) + (coeffKd * diff);
    u /= coeffK;
    if (u > 516) u = 516;
    if (u < -516) u = -516;

    // set direction and speed based on mode
    if(u>=0)
    {
        Motor0_Direction = DIR_CCW;
    }
    else
    {
        Motor0_Direction = DIR_CW;
    }
    if (abs(error) > deadBand)
        setPWM0_Duty(abs(u) + coeffKo);
    else
//        Stop_Motor0();
        setPWM0_Duty(0);

    capturePhase++;
    if (capturePhase == Tcap)
    {
        capturePhase = 0;
        captureBufferY[captureBufferIndex] = y;
        captureBufferU[captureBufferIndex++] = u;
        if (captureBufferIndex == CAPTURE_SIZE)
        {
            captureBufferIndex = 0;
        }
    }

    TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
}

uint16_t angle = 0;
float actual_angle = 0.0;
int main(void)
 {
    float raw_V = 0;
    float alpha = 0.8;
    float bat_V_raw = 0.0, bat_I_raw = 0.0, VBUS_raw = 0.0, motor1_I_raw = 0.0;

    //initFPU();
    initHW();
    initPWM();
    initADC();
    initTimer();
    initComparator();
    initQEI();
    initPID();
    init_Motor0(); //Read Motor0 Position
    while (1)
    {
       raw_encoder1 = QEIPositionGet(QEI0_BASE); //Get raw encoder ticks
       if(battery_update_flag == true) //Update Battery Data
       {
           //Calculate Raw Values
           raw_V = 0;
           bat_V_raw = (ADC0SS0_Raw[0]* VBAT)/4096.0;
           raw_V = (ADC0SS0_Raw[1]*VSUP)/4096.0;
           bat_I_raw = (raw_V - 5.0/2)/GAIN;
           VBUS_raw = (ADC0SS0_Raw[2]* VBUS)/4096.0;

           //IIR Filter
           battery_V = alpha * battery_V + (1-alpha)*bat_V_raw;
           battery_I = alpha * battery_I + (1-alpha)*bat_I_raw;
           VBUS_V = alpha * VBUS_V + (1-alpha)*VBUS_raw;

           //Calculate Power
           avg_input_power = battery_V*battery_I;
           avg_output_power = VBUS_V*(motor1_I + motor2_I);
           battery_update_flag = false;
       }
       if(motor1_update_flag == true) //Update Motor1 Data
       {
           raw_V = 0;
           raw_V = (ADC0SS1_Raw*VSUP)/4096.0; //Raw Voltage
           motor1_I_raw = (raw_V - 5.0/2)/GAIN; //Calculate Current. See README for formula
           motor1_I = alpha * motor1_I + (1-alpha)*motor1_I_raw; //IIR Filter
           motor1_update_flag = false; //Reset flag for next update
       }
       if(motor1_position_update_flag == true)
       {
           angle = (ADC0SS2_Raw*360)/4095;
           ySetPoint = angle/ENC_RES;
           actual_angle = ENC_RES*y;
       }
    }
}
