// QEI0 Library
// Jason Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL
// Target uC:       TM4C123GH6PM
// System Clock:    -

// Hardware configuration:
// Quadrature Encoder:
//   PhA0 (PD6) and PhB0 (PD7) are connected to 256 cpr optical encoder

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_gpio.h"
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_qei.h"
#include <driverlib/sysctl.h>
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"
#include "driverlib/qei.h"
#include "driverlib/interrupt.h"
#include "qei.h"

// PortD masks
#define PHA0_MASK 64
#define PHB0_MASK 128

//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// Initialize Hardware
void initQEI()
{
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

// Set position
void setQei0Position(int32_t  pos)
{
    HWREG(QEI0_BASE+ QEI_O_POS) = pos;
}

// Get position
int32_t getQei0Position()
{
    return HWREG(QEI0_BASE+ QEI_O_POS);
}
