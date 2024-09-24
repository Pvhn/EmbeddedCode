// RGB Backlight PWM Example
// Jason Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL Evaluation Board
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

// Hardware configuration:
// Red Backlight LED:
//   M0PWM3 (PB5) drives an NPN transistor that powers the red LED
// Green Backlight LED:
//   M0PWM5 (PE5) drives an NPN transistor that powers the green LED
// Blue Backlight LED:
//   M0PWM4 (PE4) drives an NPN transistor that powers the blue LED
// ST7565R Graphics LCD Display Interface:
//   MOSI on PD3 (SSI1Tx)
//   SCLK on PD0 (SSI1Clk)
//   ~CS on PD1 (SSI1Fss)
//   A0 connected to PD2

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "tm4c123gh6pm.h"
#include "graphics_lcd.h"
#include "backlight.h"
#include "wait.h"

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// Initialize Hardware
void initHw()
{
    // Configure HW to work with 16 MHz XTAL, PLL enabled, system clock of 40 MHz
    SYSCTL_RCC_R = SYSCTL_RCC_XTAL_16MHZ | SYSCTL_RCC_OSCSRC_MAIN | SYSCTL_RCC_USESYSDIV | (4 << SYSCTL_RCC_SYSDIV_S);

    // Set GPIO ports to use APB (not needed since default configuration -- for clarity)
    SYSCTL_GPIOHBCTL_R = 0;
}

//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------

int main(void)
{
	// Initialize hardware
	initHw();
    initGraphicsLcd();
    initBacklight();

    // Turn on all pixels for maximum light transmission
    drawGraphicsLcdRectangle(0, 0, 128, 64, SET);

    // Cycle through colors
	int16_t i = 0;
	uint16_t DUTY;
	DUTY = 400*0.9;
	while(true)
	{
		// Backlight off
		setBacklightRgbColor(DUTY, DUTY, DUTY);
		// Ramp from off to bright white
//	    for (i = 0; i < 400; i++)
//		{
//			setBacklightRgbColor(i, i, i);
//		}
		// Red
//		setBacklightRgbColor(1023, 0, 0);
//	    waitMicrosecond(1000000);
//		// Orange
//		setBacklightRgbColor(1023, 384, 0);
//	    waitMicrosecond(1000000);
//		// Yellow
//		setBacklightRgbColor(1023, 1023, 8);
//	    waitMicrosecond(1000000);
//	    // Green
//		setBacklightRgbColor(0, 1023, 0);
//	    waitMicrosecond(1000000);
//		// Cyan
//		setBacklightRgbColor(0, 1023, 1023);
//	    waitMicrosecond(1000000);
//		// Blue
//		setBacklightRgbColor(0, 0, 1023);
//	    waitMicrosecond(1000000);
//		// Magenta
//		setBacklightRgbColor(1023, 0, 1023);
//	    waitMicrosecond(1000000);
	}
}
