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
#include "tm4c123gh6pm.h"
#include "qei0.h"

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
void initQei0()
{
    // Enable clocks
    SYSCTL_RCGCQEI_R |= SYSCTL_RCGCQEI_R0;
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R3;
    _delay_cycles(3);

    // Configure PhA0 and PhB0 for QE
    GPIO_PORTD_LOCK_R = GPIO_LOCK_KEY;
    GPIO_PORTD_CR_R |= PHB0_MASK;
    GPIO_PORTD_DIR_R &= ~(PHA0_MASK | PHB0_MASK);
    GPIO_PORTD_AFSEL_R |= PHA0_MASK | PHB0_MASK;
    GPIO_PORTD_PCTL_R &= ~(GPIO_PCTL_PD6_M | GPIO_PCTL_PD7_M);
    GPIO_PORTD_PCTL_R |= GPIO_PCTL_PD6_PHA0 | GPIO_PCTL_PD7_PHB0;
    GPIO_PORTD_DEN_R |= PHA0_MASK | PHB0_MASK;

    // Configure QE
    QEI0_CTL_R = QEI_CTL_CAPMODE; // capture on both edges
    QEI0_MAXPOS_R = 0xFFFFFFFF;
    QEI0_CTL_R |= QEI_CTL_ENABLE;
}

// Set position
void setQei0Position(int32_t  pos)
{
    QEI0_POS_R = pos;
}

// Get position
int32_t getQei0Position()
{
    return QEI0_POS_R;
}
