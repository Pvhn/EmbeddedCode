

#ifndef CONTROL_H_
#define CONTROL_H_

#include "control.h"

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

void PORTAISR(void);
void initHW(void);
void startMotor(void);

//void initPidController();
//void pidIsr();

void PWMSetDeadBand(void);
void PWMClearDeadBand(void);
void setMotorPwm(void);
void PWM0_ISR(void);
void PWM0_OutputUpdate(uint8_t Enable);
void PWMOutputOff(void);
void setMotor0_Direction(bool dir);
void initPWM(void);

#endif /* CONTROL_H_ */





