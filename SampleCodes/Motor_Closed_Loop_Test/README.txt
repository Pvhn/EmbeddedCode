//================ Program Notes ====================
//This version of the closed loop program controls the motor position using an input from a potentionmeter
 NOTE: This version is slightly outdated as it was used sinmply to test PID implementation. Very little tuning was performed
 

It is reccommended to read the TIVA C datasheet as well as the driverlib manual for good understanding.
It is also helpful to go through the driverlib library codes and obtain an understanding of what is happening when
a certain function is called.
There are alot things that the TIVA C is picky about and thus care should be taken when programming

If any assistance is needed please contact Peter through email (peter.nguyen3@mavs.uta.edu) or through teams directly
For some information, refer to the closed loop state diagram and flowcharts
For information on I2C, please refer to the CSE deliverables

//=== GPIO ==

GPIO Port A is used for the hall sensors on motor 1
	PA2 -> HS3
	PA3 -> HS2
	PA4 -> HS1
	
//=== PWM ===
	PB6 -> PWM0 (AH)
	PB7 -> PWM1 (AL)
	PB4 -> PWM2 (BH)
	PB5 -> PWM3 (BL)
	PE4 -> PWM4 (CH)
	PE5 -> PWM5 (CL)
	
	- Currently synchronization is handled when the output is updated.
	Due to a TIVA C bug, the PWM ISR is unable to be used to perform sychnronous control of the motor
	
	- Note, when in up/down mode, the PWM period is half of what is expected. Furthermore the the period is reversed (0 = 100% and Load = 0%)
	This is seen in the PWM duty implementation
	

//=== ADC ===

	-ADC0
	-4 different sample sequencers (SSx)
	
	ADC0
	-SS3 not current enabled but can be used for overcurrent detection (via digital comparator)
	-SS1 is used for sampling the current of Motor 0. (Sample trigger via compare value = 0 on PWM Module 0)
	-SS2 is used for sampling the motor positon from the potentiometer
	-SS0 NOT RECOMMENDED unless full buffer is utilized. Using partial of the buffer results in unexpected behavior\
		(Note: in this version, SS0 is being used to sample battery voltage and current data. However, due to a tiva c bug
		the sampling is not occuring and effectly disabled)
		
	
	Notes it would be prefered to sample when the compare = Load. However, a tiva c bug is preventing from doing this. 
	//See Open Loop readme for info on digital comparator
	
	ADC1
	-SS1 is used for Battery Voltage and Current Sensing as well as VBUS voltage
		-This is triggered via a timer running at the same frequency as the PWM modules 
	
	- Each ISR sets a flag at the end of conversion to indicae in main when to update the values.
	
//=== PID ISR ===

	-This module is implemented using a timer that occurs at a rate of 10kHz. For every interrupt, the system will
	read encoder psoiton as well as the desired position from the UI. It will then calculate the appropriate PID values
	
	- A higher sample rate is prefered to improve PID stabilty. However, rates >10kHz were tested and resulted in other
	interrupts not occuring (due to the lengthy calculatinons the PID must do). Improvement of the code and optimization
	will allow for a higher sampling rate.
		
//=== Heartbeat ISR ===
	
	NOT YET IMPLEMENTED.
	-This module should be implemented using a timer. In it it will check for certain peripherals such as if a
	if a I2C flag was recieved within the last 300ms (if not something is wrong) or simply if the encoder and hall sensors
	are changing correctly when the motor is moving. 
	I will leave this up to yall on how to check everything.
	
//=== Analog Comparator ===

	-This module is used for cycle-cycle current limiting
	- It will sample the analog voltage from the current sense amplifier and output a high when the threshold is crossed.
	- The output of the comparator is directly attached to the PWM fault pin to suppress the PWM upon overcurrent conditions
	- The fault will be asserted for a minimum of 1 PWM cycle and for as long as the overcurrent condition exist.
	- Everything is handled internally and thus no clearing is neccessary as the fault will automatically clear when overcurrent
	goes away.
	
	PC7 -> COMP0 Input
	PF0 -> COMP0 Output
	PD2 -> PWM Module 0 Fault Pin
	
	
		
	
				   
				   
				   
				   
				   
				   
				   
				   
				   