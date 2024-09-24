
#include <stdbool.h>
#include <stdint.h>
#include <tm4c123gh6pm.h>

// BITBAND aliases
#define RED_LED (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 1*4)))
#define BLUE_LED (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 2*4)))
#define GREEN_LED (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 3*4)))
#define SW1  (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 4*4)))
#define SW2  (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 0*4)))

#define LEDS_MASK 0xE
#define SW1_MASK 16
#define SW2_MASK 1
#define SW_MASK 0x11

#define PWMLOAD 1024

float percent = 0.3;

// Initialize Hardware
void initHW()
{
    // Configure HW to work with 16 MHz XTAL, PLL enabled, system clock of 40 MHz
    SYSCTL_RCC_R = SYSCTL_RCC_XTAL_16MHZ | SYSCTL_RCC_OSCSRC_MAIN | SYSCTL_RCC_USESYSDIV | (4 << SYSCTL_RCC_SYSDIV_S);

    // Set GPIO ports to use APB (not needed since default configuration -- for clarity)
    SYSCTL_GPIOHBCTL_R = 0;

    // Enable Timer and GPIO PORTF peripherals
    SYSCTL_RCGCTIMER_R |= SYSCTL_RCGCTIMER_R1;
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R5;
    _delay_cycles(3);

    /* PF0 (SW2) is a special GPIO pin
     * and is locked from writing by default
     * To use, it must be unlocked by
     * writing to the GPIOLOCK and GPIOCR registers
     */
    GPIO_PORTF_LOCK_R = 0x4C4F434B;
    GPIO_PORTF_CR_R |= SW2_MASK;

    GPIO_PORTF_DIR_R &= ~SW_MASK;   // Enable PF0 and PF4 as inputs
    GPIO_PORTF_DEN_R |= SW_MASK;    // Set Digital Enable
    GPIO_PORTF_PUR_R |= SW_MASK;    // Set internal Pull-up registers

    // Configuring GPIO Interrupts
    GPIO_PORTF_IM_R &= ~SW1_MASK;       // Mask out interrupts to prevent false trip during config
    GPIO_PORTF_IS_R &= ~SW1_MASK;       // Set for interrupt on edge detect
    GPIO_PORTF_IBE_R &= ~SW1_MASK;      // Clear IBE to use only single edge detect
    GPIO_PORTF_IEV_R |= SW1_MASK;       // Set SW1 to interrupt on rising edge
    GPIO_PORTF_ICR_R |= SW1_MASK;       // Clear any pending Interrupts
    GPIO_PORTF_IM_R |= SW1_MASK;        // Turn on Switch Interrupts
    NVIC_EN0_R |= 1 << (INT_GPIOF-16);  // Enable interrupts for Port F

    // Configuring Timer Interrupt (for debounce)
    TIMER1_CTL_R &= ~TIMER_CTL_TAEN;            // turn-off timer before reconfiguring
    TIMER1_CFG_R = TIMER_CFG_32_BIT_TIMER;      // configure as 32-bit timer (A+B)
    TIMER1_TAMR_R = TIMER_TAMR_TAMR_PERIOD;     // configure for periodic mode (count down)
    TIMER1_TAILR_R = 200000;                    // set load value to 2e5 for 200 Hz interrupt rate
    TIMER1_CTL_R |= TIMER_CTL_TAEN;             // turn-on timer
    NVIC_EN0_R |= 1 << (INT_TIMER1A-16);        // turn-on interrupt 37 (TIMER1A)
}

void PORTF_ISR()
{
    if((GPIO_PORTF_MIS_R & SW1_MASK) == SW1_MASK )
    {
        // Increment the DC percentage by 20
        percent = percent + 0.2;

        // If DC percentage is greater than 90, reset to 30.
        if(percent > 0.9)
        {
            percent = 0.30;
        }

        // Clear PORTF Interrupt and disable
        // Enable Timer Interrupt for debounce
        GPIO_PORTF_IM_R &= ~SW1_MASK;
        GPIO_PORTF_ICR_R |= SW1_MASK;
        TIMER1_IMR_R = TIMER_IMR_TATOIM;
    }
}

void debounce_ISR()
{
    static uint8_t debounceCount = 0;

    debounceCount ++;

    // Debounce time is approximately (1/200Hz)*20 = 100ms
    if (debounceCount == 20)
    {
        debounceCount = 0;
        GPIO_PORTF_ICR_R |= SW1_MASK;       // Clear switch interrupts that occurred during debounce time.
        GPIO_PORTF_IM_R |= SW1_MASK;        // Re-enable switch interrupt for next press.
        TIMER1_IMR_R &= ~TIMER_IMR_TATOIM;  // Turn off timer interrupt
    }

    // Clear Timer Interrupt flag
    TIMER1_ICR_R = TIMER_ICR_TATOCINT;
}

void initPWM()
{
    // Enable clocks
    SYSCTL_RCGCPWM_R |= SYSCTL_RCGCPWM_R1;
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R5;
    _delay_cycles(3);

    // Configure three backlight LEDs
    GPIO_PORTF_DIR_R |= LEDS_MASK;      // Configure PF1,2,3 as outputs (RGB LED)
    GPIO_PORTF_DR2R_R |= LEDS_MASK;     // Set drive strength to 2mA (default)
    GPIO_PORTF_DEN_R |= LEDS_MASK;      // Digital Enable
    GPIO_PORTF_AFSEL_R |= LEDS_MASK;    // Auxiliary Function Enable (PWM)
    GPIO_PORTF_PCTL_R &= 0xFFF0;        // Enable PWM for PF1,2,3
    GPIO_PORTF_PCTL_R |= 0x5550;        // Configure as PWM for PF1,2,3

    SYSCTL_SRPWM_R = SYSCTL_SRPWM_R1;   // Reset PWM1 Module
    SYSCTL_SRPWM_R = 0;
    PWM1_2_CTL_R = 0;   // Ensure PWM1 Generator 2 and 3 are disabled before configuring
    PWM1_3_CTL_R = 0;

    // Set PWM to set high when load value is equal to comparator value
    PWM1_2_GENB_R = PWM_2_GENB_ACTCMPBD_ZERO | PWM_2_GENB_ACTLOAD_ONE;
    PWM1_3_GENA_R = PWM_3_GENA_ACTCMPAD_ZERO | PWM_3_GENA_ACTLOAD_ONE;
    PWM1_3_GENB_R = PWM_3_GENB_ACTCMPBD_ZERO | PWM_3_GENB_ACTLOAD_ONE;

    // Set PWM Period and invert the PWM output
    PWM1_2_LOAD_R = PWMLOAD;
    PWM1_3_LOAD_R = PWMLOAD;
    PWM1_INVERT_R = PWM_INVERT_PWM5INV | PWM_INVERT_PWM6INV | PWM_INVERT_PWM7INV;

    // Set PWM outputs to 0
    PWM1_2_CMPB_R = 0;
    PWM1_3_CMPA_R = 0;
    PWM1_3_CMPB_R = 0;

    // Enable PWM1 Generator 2 and 3
    PWM1_2_CTL_R = PWM_1_CTL_ENABLE;
    PWM1_3_CTL_R = PWM_1_CTL_ENABLE;
    PWM1_ENABLE_R = PWM_ENABLE_PWM5EN | PWM_ENABLE_PWM6EN| PWM_ENABLE_PWM7EN;
}

/**
 * main.c
 */
int main(void)
{
    initHW();
    initPWM();

    uint16_t duty = PWMLOAD*percent;
    uint8_t toggle = false;

    while(true)
    {
        // If SW2 is pressed then continuously ramp up/down the LED.
        if(!SW2)
        {
            // If ramp down
            if(toggle)
            {
                // Ramp down (Exhale)
                PWM1_3_CMPA_R = PWMLOAD-duty;
            }
            else
            {
                // Ramp up (Inhale)
                PWM1_3_CMPA_R = duty;
            }

            // Increment duty cycle by 1
            duty++;

            // If duty cycle is greater than max reset to 0
            // and switch ramp sequence
            if(duty>=PWMLOAD)
            {
                duty = 0;
                toggle = toggle^1;
            }

            // Delay for approximately 1ms.
            _delay_cycles((40e6)*0.001);
        }
        else
        {
            // Set DC to current percent DC
            duty = PWMLOAD*percent;

            // Toggle between On and Off
            if(toggle)
            {
                PWM1_3_CMPA_R = 0;
            }
            else
            {
                PWM1_3_CMPA_R = duty;
            }
            toggle = toggle ^ 1;

            // Delay for 0.5s (2Hz)
            _delay_cycles((40e6)*0.5);
        }
    }
}
