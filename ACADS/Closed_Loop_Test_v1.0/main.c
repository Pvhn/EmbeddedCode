
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "inc/hw_i2c.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_ints.h"
#include "driverlib/i2c.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/interrupt.h"
#include "control.h"
#include "adc.h"
#include "qei.h"

bool Hall_Stopped = true;
bool Hall_State_Unknown = true;


// Function definition
void PWM_update(unsigned char Next_Hall_Sequence);
void Start_Motor(void);
void Stop_Motor(void);



//expansion board - tiva c
//GND 1C - GND
//S_Clock 2C - PB2
//S_Data 3C - PB3



#define SYSCTL_GPIOHBCTL_R      (*((volatile unsigned long *)0x400FE06C))
#define SYSCTL_RCGCGPIO_R       (*((volatile unsigned long *)0x400FE608))
#define GPIO_PORTA_DIR_R        (*((volatile unsigned long *)0x40004400))
#define GPIO_PORTA_DEN_R        (*((volatile unsigned long *)0x4000451C))
#define GPIO_PORTA_PUR_R        (*((volatile unsigned long *)0x40004510))

#define GPIO_PORTF_DIR_R        (*((volatile unsigned long *)0x40025400))
#define GPIO_PORTF_DR2R_R       (*((volatile unsigned long *)0x40025500))
#define GPIO_PORTF_DEN_R        (*((volatile unsigned long *)0x4002551C))

#define PUSH_BUTTON_0 (*((volatile uint32_t *)(0x42000000 + (0x400043FC-0x40000000)*32 + 2*4)))
#define BLUE_LED     (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 2*4)))

#define PB2 4
#define BLUE_LED_MASK 4

#define avg_power_update 150
#define avg_braking_update 151

#define encoder_1_update 160
#define encoder_2_update 161
#define hall_sensor_1_update 162
#define hall_sensor_2_update 163
#define status_motor_1_update 164
#define status_motor_2_update 165
#define micro_controller_1_update 166
#define micro_controller_2_update 167

#define power_supply_1_update 170
#define power_supply_2_update 171
#define power_supply_3_update 172
#define mega_cap_1_update 173
#define mega_cap_2_update 174
#define breaking_circuit_1_update 175
#define breaking_circuit_2_update 176

#define motor_1_update 120
#define motor_2_update 110
#define motor_1_update_neg 121
#define motor_2_update_neg 111

#define func_test 130
#define is_idle 140

// Braking voltage recovered


uint8_t dataRead = 0;
bool update_stop_flag = false;
int8_t return_vals[21];

int8_t motor_position[2];

uint8_t avg_input_power = 22;
uint8_t power_generated = 33;

uint8_t encoder_1 = 0;
uint8_t encoder_2 = 0;
uint8_t hall_sensor_1 = 0;
uint8_t hall_sensor_2 = 0;
uint8_t motor_1 = 0;
uint8_t motor_2 = 0;
uint8_t micro_controller_1 = 0;
uint8_t micro_controller_2 = 0;

uint8_t power_supply_1 = 0;
uint8_t power_supply_2 = 0;
uint8_t power_supply_3 = 0;
uint8_t mega_cap_1 = 0;
uint8_t mega_cap_2 = 0;
uint8_t breaking_circuit_1 = 0;
uint8_t breaking_circuit_2 = 0;


uint8_t motor_index = 5;
uint8_t loop_count = 0;
uint8_t func_test_result = 1;

//initialize I2C module 0
//Slightly modified version of TI's example code
void InitI2C0()
{
    SYSCTL_GPIOHBCTL_R = 0;
    SYSCTL_RCGCGPIO_R |= 0x1 | 0x20;
    _delay_cycles(3);

    GPIO_PORTF_DIR_R |= BLUE_LED_MASK;   // bit 2 as output, other pins are inputs
    GPIO_PORTF_DR2R_R |= BLUE_LED_MASK;   // set drive strength to 2mA (not needed since default configuration -- for clarity)
    GPIO_PORTF_DEN_R |= BLUE_LED_MASK;   // enable LED

    GPIO_PORTA_DIR_R &= ~(PB2);
    GPIO_PORTA_DEN_R |= PB2;

    GPIO_PORTA_PUR_R |= PB2;  // For a push button to PB2
    //enable I2C module 0
    SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C0);

    //reset module
    SysCtlPeripheralReset(SYSCTL_PERIPH_I2C0);

    //enable GPIO peripheral that contains I2C 0
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);

    // Configure the pin muxing for I2C0 functions on port B2 and B3.
    GPIOPinConfigure(GPIO_PB2_I2C0SCL);
    GPIOPinConfigure(GPIO_PB3_I2C0SDA);

    // Select the I2C function for these pins.
    GPIOPinTypeI2CSCL(GPIO_PORTB_BASE, GPIO_PIN_2);
    GPIOPinTypeI2C(GPIO_PORTB_BASE, GPIO_PIN_3);

    // Enable and initialize the I2C0 master module.  Use the system clock for
    // the I2C0 module.  The last parameter sets the I2C data transfer rate.
    // If false the data rate is set to 100kbps and if true the data rate will
    // be set to 400kbps.

}

// The interrupt handler for the for I2C0 data slave interrupt.
void I2C0SlaveIntHandler(void)
{
    //Tivaware i2c library reference
    //https://www.ti.com/lit/ug/spmu298d/spmu298d.pdf?ts=1588793931897#page=335&zoom=100,84,700

    // Clear the I2C0 interrupt flag.
    I2CSlaveIntClear(I2C0_BASE);

    uint32_t status = I2CSlaveStatus(I2C0_BASE);

    switch(status){

    case I2C_SLAVE_ACT_NONE:
        break;

    case I2C_SLAVE_ACT_RREQ:
        break;

    case I2C_SLAVE_ACT_TREQ:
        if (loop_count == 1) // Loops through and returns to the server the states and values of the system.
        {
            I2CSlaveDataPut(I2C0_BASE, return_vals[0]);
        }
        if (loop_count == 2)
        {
            I2CSlaveDataPut(I2C0_BASE, return_vals[1]);
        }
        if (loop_count == 3)
        {
            I2CSlaveDataPut(I2C0_BASE, return_vals[2]);
        }
        if (loop_count == 4)
        {
            I2CSlaveDataPut(I2C0_BASE, return_vals[3]);
        }
        if (loop_count == 5)
        {
            I2CSlaveDataPut(I2C0_BASE, return_vals[4]);
        }
        if (loop_count == 6)
        {
            I2CSlaveDataPut(I2C0_BASE, return_vals[5]);
        }
        if (loop_count == 7)
        {
            I2CSlaveDataPut(I2C0_BASE, return_vals[6]);
        }
        if (loop_count == 8)
        {
            I2CSlaveDataPut(I2C0_BASE, return_vals[7]);
        }
        if (loop_count == 9)
        {
            I2CSlaveDataPut(I2C0_BASE, return_vals[8]);
        }
        if (loop_count == 10)
        {
            I2CSlaveDataPut(I2C0_BASE, return_vals[9]);
        }
        if (loop_count == 11)
        {
            I2CSlaveDataPut(I2C0_BASE, return_vals[10]);
        }
        if (loop_count == 12)
        {
            I2CSlaveDataPut(I2C0_BASE, return_vals[11]);
        }
        if (loop_count == 13)
        {
            I2CSlaveDataPut(I2C0_BASE, return_vals[12]);
        }
        if (loop_count == 14)
        {
            I2CSlaveDataPut(I2C0_BASE, return_vals[13]);
        }
        if (loop_count == 15)
        {
            I2CSlaveDataPut(I2C0_BASE, return_vals[14]);
        }
        if (loop_count == 16)
        {
            I2CSlaveDataPut(I2C0_BASE, return_vals[15]);
        }
        if (loop_count == 17)
        {
            I2CSlaveDataPut(I2C0_BASE, return_vals[16]);
        }
        if (loop_count == 18)
        {
            I2CSlaveDataPut(I2C0_BASE, return_vals[17]);
        }
        if (loop_count == 19)
        {
            I2CSlaveDataPut(I2C0_BASE, return_vals[18]);
            loop_count = 0;
            update_stop_flag = false;
        }
        if (loop_count == 20)
        {
            I2CSlaveDataPut(I2C0_BASE, func_test_result);
            loop_count = 0;
            update_stop_flag = false;
        }
        break;

    case I2C_SLAVE_ACT_RREQ_FBR:
        dataRead = I2CSlaveDataGet(I2C0_BASE);

        if ((motor_index == 5) & (dataRead < 10)) // This is the default routine. If no user changes occur then the system will send the return_vals array in order.
         {
             update_stop_flag = true;
             loop_count = loop_count + 1;
             break;
         }

        if((motor_index == 5) & (dataRead == avg_power_update))
        {
            update_stop_flag = true;
            loop_count = 3;
            break;
        }

        if((motor_index == 5) & (dataRead == avg_braking_update))
        {
            update_stop_flag = true;
            loop_count = 4;
            break;
        }
        if((motor_index == 5) & (dataRead == encoder_1_update))
         {
             update_stop_flag = true;
             loop_count = 5;
             break;
         }
        if((motor_index == 5) & (dataRead == encoder_2_update))
         {
             update_stop_flag = true;
             loop_count = 6;
             break;
         }
        if((motor_index == 5) & (dataRead == hall_sensor_1_update))
         {
             update_stop_flag = true;
             loop_count = 7;
             break;
         }
        if((motor_index == 5) & (dataRead == hall_sensor_2_update))
         {
             update_stop_flag = true;
             loop_count = 8;
             break;
         }
        if((motor_index == 5) & (dataRead == status_motor_1_update))
         {
             update_stop_flag = true;
             loop_count = 9;
             break;
         }
        if((motor_index == 5) & (dataRead == status_motor_2_update))
         {
             update_stop_flag = true;
             loop_count = 10;
             break;
         }
        if((motor_index == 5) & (dataRead == micro_controller_1_update))
         {
             update_stop_flag = true;
             loop_count = 11;
             break;
         }
        if((motor_index == 5) & (dataRead == micro_controller_2_update))
        {
            update_stop_flag = true;
            loop_count = 12;
            break;
        }
        if((motor_index == 5) & (dataRead == power_supply_1_update))
        {
            update_stop_flag = true;
            loop_count = 13;
            break;
        }
        if((motor_index == 5) & (dataRead == power_supply_2_update))
        {
            update_stop_flag = true;
            loop_count = 14;
            break;
        }
        if((motor_index == 5) & (dataRead == power_supply_3_update))
        {
            update_stop_flag = true;
            loop_count = 15;
            break;
        }
        if((motor_index == 5) & (dataRead == mega_cap_1_update))
        {
            update_stop_flag = true;
            loop_count = 16;
            break;
        }
        if((motor_index == 5) & (dataRead == mega_cap_2_update))
        {
            update_stop_flag = true;
            loop_count = 17;
            break;
        }
        if((motor_index == 5) & (dataRead == breaking_circuit_1_update))
        {
            update_stop_flag = true;
            loop_count = 18;
            break;
        }
        if((motor_index == 5) & (dataRead == breaking_circuit_2_update))
        {
            update_stop_flag = true;
            loop_count = 19;
            break;
        }
        if ((motor_index == 5) & (dataRead == func_test))
        {
            // Perform functionality test
            update_stop_flag = true;
            loop_count = 13;
            break;
        }
        if ((motor_index == 5) & (dataRead == is_idle))
        {
            update_stop_flag = true; // Reseting motor positions for idle state
            motor_position[0] = 0;
            motor_position[1] = 0;
            update_stop_flag = false;
            break;
        }

        if ((dataRead == motor_1_update) & (motor_index == 5)) // This statement determines if motor 1 needs to be written to, if its POSITIVE, and prepares for the next write.
        {
            update_stop_flag = true; // Stops updating values of the system
            motor_index = 0; // Sets the motor which it wants to update. motor 1 being the 0th index.
            break;
        }

        if ((dataRead == motor_2_update) & (motor_index == 5)) // Same as above except motor 2 POSITIVE.
        {
            update_stop_flag = true;
            motor_index = 1; // Motor 2 is at index 1 of the array.
            break;
        }

        if ((dataRead == motor_1_update_neg) & (motor_index == 5)) // This statement determine if motor 1 needs to be written to, if its NEGATIVE, and prepares for the next write.
        {
            update_stop_flag = true;
            motor_index = 2; // Sets the motor which will be updated. Will be subtracted by 2 later.
            break;
        }

        if ((dataRead == motor_2_update_neg) & (motor_index == 5)) // Same as above except for motor 2 NEGATIVE.
        {
            update_stop_flag = true;
            motor_index = 3;
            break;
        }

        if ((motor_index == 0) & (dataRead < 41))
        {
            motor_position[motor_index] = dataRead; // Writing the new angle to the appropriate registers for motor 1 POSITIVE.
            motor_index = 5; // Reset motor_index.
            update_stop_flag = false; // Reset the update flag.
            break;
        }

        if ((motor_index == 2) & (dataRead < 41))
        {
            motor_position[motor_index - 2] = dataRead * -1; // Writing the new angle to the appropriate registers for motor 1 NEGATIVE.
            motor_index = 5; // Reset motor_index.
            update_stop_flag = false; // Reset the update flag.
            break;
        }

        if ((motor_index == 1) & (dataRead < 41))
        {
            motor_position[motor_index] = dataRead; // Writing the new angle to the appropriate registers for motor 2 POSITIVE.
            motor_index = 5; // Reset motor_index
            update_stop_flag = false; // Reset the update flag
            break;
        }

        if ((motor_index == 3) & (dataRead < 41))
        {
            motor_position[motor_index - 2] = dataRead * -1; // Writing the new angle to the appropriate registers for motor 2 NEGATIVE.
            motor_index = 5; // Reset motor_index
            update_stop_flag = false; // Reset the update flag
            break;
        }

        break;

    case I2C_SLAVE_ACT_OWN2SEL:     break;
    case I2C_SLAVE_ACT_QCMD:        break;
    case I2C_SLAVE_ACT_QCMD_DATA:   break;

    default: //unsupported case
                                    break;
    }
}

int main(void)
{
    initHW(); //Init Hall Sensors
    initADC(); //Init ADC
    initPWM(); //Init PWM
    initQEI();
    InitI2C0();

    IntEnable(INT_I2C0);
    I2CSlaveIntEnableEx(I2C0_BASE, I2C_SLAVE_INT_DATA);

    I2CSlaveEnable(I2C0_BASE);

    I2CSlaveInit(I2C0_BASE, 0x1d);

    IntMasterEnable();

   while (1)
   {
       if(update_stop_flag == false)
       {
           return_vals[0] = motor_position[0]; // Registers that have motor 1 position
           return_vals[1] = motor_position[1]; // Registers that have motor 2 position
           return_vals[2] = avg_input_power; // Register holding avg_input_power
           return_vals[3] = power_generated; // Register holding breaking circuit power
           return_vals[4] = encoder_1;
           return_vals[5] = encoder_2;
           return_vals[6] = hall_sensor_1;
           return_vals[7] = hall_sensor_2;
           return_vals[8] = motor_1;
           return_vals[9] = motor_2;
           return_vals[10] = micro_controller_1;
           return_vals[11] = micro_controller_2;
           return_vals[12] = power_supply_1;
           return_vals[13] = power_supply_2;
           return_vals[14] = power_supply_3;
           return_vals[15] = mega_cap_1;
           return_vals[16] = mega_cap_2;
           return_vals[17] = breaking_circuit_1;
           return_vals[18] = breaking_circuit_2;
       }
   }
}
