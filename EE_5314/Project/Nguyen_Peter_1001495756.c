//#define DEBUG //Uncomment to debug

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "tm4c123gh6pm.h"
#include "wait.h"
#include "Uart0.h"
#include "project.h"
#include "eeprom.h"

//Masked Ports
#define ALL_LEDS (*((volatile uint32_t *)(0x40025000 + 0x0E*4)))

//Bitband Alias
#define RED_LED (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 1*4)))
#define BLUE_LED (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 2*4)))
#define GREEN_LED (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 3*4)))
#define PC4 (*((volatile uint32_t *)(0x42000000 + (0x400063FC-0x40000000)*32 + 4*4)))
#define PC5 (*((volatile uint32_t *)(0x42000000 + (0x400063FC-0x40000000)*32 + 5*4)))
#define PC7 (*((volatile uint32_t *)(0x42000000 + (0x400063FC-0x40000000)*32 + 7*4)))

//DMX Defines
#define DE PC7
#define MAX_ADDR 512
#define MAX_QUEUE 10
//Mask
// PortF Mask
#define ALL_LED_MASK 14
#define GREEN_LED_MASK 8
#define BLUE_LED_MASK 4
#define RED_LED_MASK 2
// PortC mask
#define UART1_RX_MASK 16
#define UART1_TX_MASK 32
#define PC7_MASK 128

//Magic Numbers/char
#define BUFF_SIZE 80
#define FCY 40e6 //Clock Cycle

//Non-printable characters (Used in getsUart)
#define BCKSPACE 8
#define DEL 127

#define MODE_ADDR 600
#define DEVICE_ADDR 601

enum Mode
{
    Controller = 0, Device = 1
};

//Global Variables
char bufferTX[BUFF_SIZE];
bool run, poll_mode;
bool valid = false;
uint8_t wr_index = 0, rd_index = 0;
uint16_t Phase, max_addr = 512, Device_Addr = 1;;
uint8_t DMX_data[MAX_ADDR];
uint8_t Mode = Controller;
uint8_t LED_Timeout;
uint8_t init_val[MAX_QUEUE], final_val[MAX_QUEUE], signal[MAX_QUEUE];
uint16_t Addr[MAX_QUEUE], time_N[MAX_QUEUE];


void displayUart0(char str[])
{
    int i;
    bool bufferFull;
    bufferFull = (wr_index + 1) % BUFF_SIZE == rd_index;
    for (i = 0; str[i] != '\0'; i++)
    {
        if (!bufferFull) //If buffer is not full
        {
            bufferTX[wr_index] = str[i]; //Copy str to buffer
            wr_index = (wr_index + 1) % BUFF_SIZE;
        }
    }
    if (UART0_FR_R & UART_FR_TXFE) //If Uart0 TX FIFO empty
    {
        UART0_DR_R = bufferTX[rd_index]; //Send first char directly to FIFO
        rd_index = (rd_index + 1) % BUFF_SIZE;
    }
}

void getsUart0(char *str_Buffer)
{
    uint8_t count = 0;
    char c;
    while (1)
    {
        c = getcUart0();
        BLUE_LED = 1;
        LED_Timeout = 1;
        displayUart0(&c);
        if (c == DEL || c == BCKSPACE & count > 0)
        {
            count--;
//            displayUart0(" ");
            continue;
        }
        else if (c == DEL || c == BCKSPACE & count == 0)
        {
            continue;
        }
        else
        {
            if (c == '\r')
            {
                str_Buffer[count] = '\0';
//                displayUart0(&c);
                displayUart0("\r\n");
                break;
            }
            else if (c >= ' ')
            {
                str_Buffer[count++] = c;
                if (count == MAX_CHARS)
                {
                    str_Buffer[count] = '\0';
                    displayUart0("\r\n");
                    break;
                }
            }
        }
    }
}

void setUart0ISR(void)
{
    UART0_CTL_R = 0;
    UART0_IM_R = UART_IM_TXIM; //Enable TX interrupt
    NVIC_EN0_R |= 1 << (INT_UART0 - 16); //Enable interrupt 5 (UART0)
    UART0_CTL_R = UART_CTL_EOT | UART_CTL_TXE | UART_CTL_RXE | UART_CTL_UARTEN; //Enable UART and EOT bit to trigger UARTISR upon TX FIFO empty
}

void UART0ISR()
{
    bool bufferEmpty;
    bufferEmpty = (wr_index == rd_index);
    if (!bufferEmpty)
    {
        UART0_DR_R = bufferTX[rd_index];
        rd_index = (rd_index + 1) % BUFF_SIZE;
    }
    UART0_ICR_R = UART_ICR_TXIC; // Clear TX interrupt flag
}

void initTimer()
{
    // Configure Timer 1 for keyboard service
    SYSCTL_RCGCTIMER_R |= SYSCTL_RCGCTIMER_R0 | SYSCTL_RCGCTIMER_R1 | SYSCTL_RCGCTIMER_R2;
    TIMER0_CTL_R &= ~TIMER_CTL_TAEN;      // turn-off timer before reconfiguring
    TIMER1_CTL_R &= ~TIMER_CTL_TAEN;
    TIMER2_CTL_R &= ~TIMER_CTL_TAEN;
    TIMER0_CFG_R = TIMER_CFG_16_BIT;    // configure as 16-bit timer (A+B)
    TIMER1_CFG_R = TIMER_CFG_32_BIT_TIMER;
    TIMER2_CFG_R = TIMER_CFG_32_BIT_TIMER;
    TIMER0_TAMR_R = TIMER_TAMR_TAMR_1_SHOT; // configure for One-Shot Mode
    TIMER1_TAMR_R = TIMER_TAMR_TAMR_PERIOD;
    TIMER2_TAMR_R = TIMER_TAMR_TAMR_PERIOD;
    TIMER0_TAILR_R = 7040;    // set load value to 7040 for 176us interrupt rate
    TIMER1_TAILR_R = 400000;
    TIMER2_TAILR_R = 400000;//
    NVIC_EN0_R |= 1 << (INT_TIMER0A - 16);      // Enable interrupt 19 (TIMER1A)
    NVIC_EN0_R |= 1 << (INT_TIMER1A - 16);
    NVIC_EN0_R |= 1 << (INT_TIMER2A - 16);
    TIMER0_IMR_R = TIMER_IMR_TATOIM;                 // turn-on Timer interrupt
    TIMER1_IMR_R = TIMER_IMR_TATOIM;
    TIMER2_IMR_R = TIMER_IMR_TATOIM;

}

//Set timer length
void startTimer(float time)
{
    uint16_t Ld_value;
    Ld_value = FCY * time;
    TIMER0_CTL_R &= ~TIMER_CTL_TAEN;      // turn-off timer before reconfiguring
    TIMER0_TAILR_R = Ld_value;
    TIMER0_CTL_R |= TIMER_CTL_TAEN;     // Start Timer
}

void send_break(void)
{
    UART1_IM_R &= ~UART_IM_RXIM;
    DE = 1;
    GPIO_PORTC_AFSEL_R &= ~UART1_TX_MASK; //GPIO Control
    PC5 = 0;
    startTimer(176e-6);
}
void start_DMXTX(void)
{
    Phase = 0;
    send_break();
}

void start_DMXRX(void)
{
    run = false;
    DE = 0;
    UART1_IM_R |= UART_IM_RXIM; //Enable RX
    UART1_IM_R &= ~UART_IM_TXIM; //Disable TX

}

void start_Polling(void)
{
    run = false;
    poll_mode = true;
    Phase = 0;
    send_break();

}

void Timer0ISR()// Handle DMX TX
{
    if (Phase == 0)
    {
        PC5 = 1;
        Phase = 1;
        startTimer(12e-6);
    }
    else if (Phase == 1 ) //Phase = 1
    {
        GPIO_PORTC_AFSEL_R |= UART1_TX_MASK; //UART Control
        Phase = 2;
        UART1_IM_R = UART_IM_TXIM; //Enable Uart1 TX Interrupt
        if(poll_mode == true)
        {
            UART1_DR_R = 0xF6;
        }
        else
        {
            UART1_DR_R = 0; //Write start bit to Uart1
        }
    }
    TIMER0_ICR_R = TIMER_ICR_TATOCINT;               // clear interrupt flag

}

void Timer1ISR() //Handle Status LEDs
{
   switch(Mode)
   {
   case Controller:
       if(LED_Timeout >0)
           LED_Timeout--;
       else if(LED_Timeout == 0)
       {
           BLUE_LED = 0;
           GREEN_LED = 0;
           RED_LED = 1;
       }
       break;
   case Device:
       if(LED_Timeout >0)
           LED_Timeout--;
       else if(LED_Timeout == 0)
       {
           GREEN_LED = 0;
           RED_LED = 0;
       }
       break;
   }
   TIMER1_ICR_R |= TIMER_ICR_TATOCINT;
}

void Timer2ISR() //Handle Pattern Que
{
    static uint16_t delta_N[MAX_QUEUE];

    if(Addr[0]>0)
    {
        delta_N[0]++; //Keeps track of elapsed time
        if(delta_N[0] == time_N[0]) //Timeout situation
        {
            DMX_data[Addr[0]] = final_val[0]; //Write V2 to DMX/device
            Addr[0] = 0; //Set Addr to 0 to stop checl.
            delta_N[0] = 0;
        }
        else
        {
            if(signal[0] == 'R') //Ramp
            {
                DMX_data[Addr[0]] = init_val[0]+((final_val[0] - init_val[0])*delta_N[0])/time_N[0];
            }
        }
    }

    if(Addr[1]>0)
    {
        delta_N[1]++;
        if(delta_N[1] == time_N[1])
        {
            DMX_data[Addr[1]] = final_val[1];
            Addr[1] = 0;
            delta_N[1] = 0;
        }
        else
        {
            if(signal[1] == 'R') //Ramp
            {
                DMX_data[Addr[1]] = init_val[1]+((final_val[1] - init_val[1])*delta_N[1])/time_N[1];
            }
        }
    }

    if(Addr[2]>0)
    {
        delta_N[2]++;
        if(delta_N[2] == time_N[2])
        {
            DMX_data[Addr[2]] = final_val[2];
            Addr[2] = 0;
            delta_N[2] = 0;
        }
        else
        {
            if(signal[2] == 'R') //Ramp
            {
                DMX_data[Addr[2]] = init_val[2]+((final_val[2] - init_val[2])*delta_N[2])/time_N[2];
            }
        }
    }

    if(Addr[3]>0)
    {
        delta_N[3]++;
        if(delta_N[3] == time_N[3])
        {
            DMX_data[Addr[3]] = final_val[3];
            Addr[3] = 0;
            delta_N[3] = 0;
        }
        else
        {
            if(signal[3] == 'R') //Ramp
            {
                DMX_data[Addr[3]] = init_val[3]+((final_val[3] - init_val[3])*delta_N[3])/time_N[3];
            }
        }
    }

    if(Addr[4]>0)
        {
            delta_N[4]++;
            if(delta_N[4] == time_N[4])
            {
                DMX_data[Addr[4]] = final_val[4];
                Addr[4] = 0;
                delta_N[4] = 0;
            }
            else
            {
                if(signal[4] == 'R') //Ramp
                {
                    DMX_data[Addr[4]] = init_val[4]+((final_val[4] - init_val[4])*delta_N[4])/time_N[4];
                }
            }
        }

    if(Addr[5]>0)
        {
            delta_N[5]++;
            if(delta_N[5] == time_N[5])
            {
                DMX_data[Addr[5]] = final_val[5];
                Addr[5] = 0;
                delta_N[5] = 0;
            }
            else
            {
                if(signal[5] == 'R') //Ramp
                {
                    DMX_data[Addr[5]] = init_val[5]+((final_val[5] - init_val[5])*delta_N[5])/time_N[5];
                }
            }
        }

    if(Addr[6]>0)
        {
            delta_N[6]++;
            if(delta_N[6] == time_N[6])
            {
                DMX_data[Addr[6]] = final_val[6];
                Addr[6] = 0;
                delta_N[6] = 0;
            }
            else
            {
                if(signal[6] == 'R') //Ramp
                {
                    DMX_data[Addr[6]] = init_val[6]+((final_val[6] - init_val[6])*delta_N[6])/time_N[6];
                }
            }
        }

    if(Addr[7]>0)
        {
            delta_N[7]++;
            if(delta_N[7] == time_N[7])
            {
                DMX_data[Addr[7]] = final_val[7];
                Addr[7] = 0;
                delta_N[7] = 0;
            }
            else
            {
                if(signal[7] == 'R') //Ramp
                {
                    DMX_data[Addr[7]] = init_val[7]+((final_val[7] - init_val[7])*delta_N[7])/time_N[7];
                }
            }
        }

    if(Addr[8]>0)
        {
            delta_N[8]++;
            if(delta_N[8] == time_N[8])
            {
                DMX_data[Addr[8]] = final_val[8];
                Addr[8] = 0;
                delta_N[8] = 0;
            }
            else
            {
                if(signal[8] == 'R') //Ramp
                {
                    DMX_data[Addr[8]] = init_val[8]+((final_val[8] - init_val[8])*delta_N[8])/time_N[8];
                }
            }
        }

    if(Addr[9]>0)
        {
            delta_N[9]++;
            if(delta_N[9] == time_N[9])
            {
                DMX_data[Addr[9]] = final_val[9];
                Addr[9] = 0;
                delta_N[9] = 0;
            }
            else
            {
                if(signal[9] == 'R') //Ramp
                {
                    DMX_data[Addr[9]] = init_val[9]+((final_val[9] - init_val[9])*delta_N[9])/time_N[9];
                }
            }
        }
    TIMER2_ICR_R |= TIMER_ICR_TATOCINT;
}

void UART1ISR()
{
    uint32_t data;
    static uint16_t rx_Phase;
    static uint8_t old_DMX_data[MAX_ADDR];
    switch(Mode)
    {
    case Controller:
        if (Phase - 2 <= max_addr)
        {
            if(poll_mode == true)
            {
                UART1_DR_R = 1;
            }
            else
            {
                UART1_DR_R = DMX_data[Phase - 1];
            }
            Phase++;
        }
        else
        {
            if(poll_mode == true)
            {
                poll_mode = false;
                displayUart0("\tPolling stopped\r\n\n");
            }
            if (run)
            {
                RED_LED = 1;
                UART1_IM_R &= ~UART_IM_TXIM;
                start_DMXTX();
            }
            else
            {
                RED_LED = 0;
            }
        }
        break;
    case Device:

        if (!(UART1_FR_R & UART_FR_RXFE))
        {
            data = UART1_DR_R;
        }
        if (UART_DR_BE & data)
        {
            rx_Phase = 0;
        }
        else
        {
            if(DMX_data[0] == 0xF6) //Polling Command from Master
            {
                poll_mode = true;
                data = UART1_DR_R & 0xFF;
                if(data == 1 && rx_Phase == Device_Addr)
                {
                    poll_mode =false;
                    DMX_data[0] = 0;
                    displayUart0("\tAck Sent\r\n");
                    RED_LED = 1;
                    LED_Timeout = 10;
                }
            }
            else
            {
                DMX_data[rx_Phase] = UART1_DR_R & 0xFF;
                PWM1_3_CMPA_R = DMX_data[Device_Addr];
                if(old_DMX_data[rx_Phase] != DMX_data[rx_Phase])
                {
                    old_DMX_data[rx_Phase] = DMX_data[rx_Phase];
                }
                else
                {
                    GREEN_LED = 1;
                    LED_Timeout = 100; //10ms *100 = 1
                }
            }
             rx_Phase++;
        }
        break;
    }
    UART1_ICR_R |= UART_ICR_RXIC;
    UART1_ICR_R |= UART_ICR_TXIC;
}

void initUart1()
{
    // Enable clocks
    SYSCTL_RCGCUART_R |= SYSCTL_RCGCUART_R1;
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R2;
    _delay_cycles(3);

    // Configure UART0 pins
    GPIO_PORTC_DIR_R |= UART1_TX_MASK;          // enable output on UART1 TX pin
    GPIO_PORTC_DIR_R &= ~UART1_RX_MASK;          // enable input on UART1 RX pin
    GPIO_PORTC_DR2R_R |= UART1_TX_MASK; // set drive strength to 2mA (not needed since default configuration -- for clarity)
    GPIO_PORTC_DEN_R |= UART1_TX_MASK | UART1_RX_MASK; // enable digital on UART1 pins
    GPIO_PORTC_AFSEL_R |= UART1_TX_MASK | UART1_RX_MASK; // use peripheral to drive PC4, PC5
    GPIO_PORTC_PCTL_R &= ~(GPIO_PCTL_PC4_M | GPIO_PCTL_PC5_M); // clear bits 0-7
    GPIO_PORTC_PCTL_R |= GPIO_PCTL_PC5_U1TX | GPIO_PCTL_PC4_U1RX;

    // Configure UART0 to 250000 baud, 8N2 format
    UART1_CTL_R = 0;                 // turn-off UART0 to allow safe programming
    UART1_CC_R = UART_CC_CS_SYSCLK;                 // use system clock (40 MHz)
    UART1_IBRD_R = 10;                                // r = 40 MHz / (Nx250kHz)
    UART1_FBRD_R = 0;                                  //
    UART1_LCRH_R = UART_LCRH_WLEN_8 | UART_LCRH_STP2; // configure for 8N2 w/ 16-level FIFO

    //Interupts
    UART1_IM_R |= UART_IM_TXIM; //Enable TX interrupt
    NVIC_EN0_R |= 1 << (INT_UART1 - 16); //Enable interrupt 6 (UART1)
    UART1_CTL_R = UART_CTL_EOT | UART_CTL_TXE | UART_CTL_RXE | UART_CTL_UARTEN;
    // enable TX, RX, and module
}

void blinkLED()
{
    GREEN_LED = 1;
    waitMicrosecond(500e3);
    GREEN_LED = 0;
}

void initPWM()
{
    // Enable clocks
    SYSCTL_RCGCPWM_R |= SYSCTL_RCGCPWM_R1;
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R5;
    _delay_cycles(3);

    // Configure three backlight LEDs
    GPIO_PORTF_DIR_R |= BLUE_LED_MASK;                       // make bit5 an output
    GPIO_PORTF_DR2R_R |= BLUE_LED_MASK;                      // set drive strength to 2mA
    GPIO_PORTF_DEN_R |= BLUE_LED_MASK;                       // enable digital
    GPIO_PORTF_AFSEL_R |= BLUE_LED_MASK;                   // select auxilary function
    GPIO_PORTF_PCTL_R &= GPIO_PCTL_PF2_M;                      // enable PWM
    GPIO_PORTF_PCTL_R |= GPIO_PCTL_PF2_M1PWM6;

    SYSCTL_SRPWM_R = SYSCTL_SRPWM_R1;                // reset PWM0 module
    SYSCTL_SRPWM_R = 0;                              // leave reset state
    PWM1_3_CTL_R = 0;                                // drives 0 and 1

    PWM1_3_GENA_R = PWM_3_GENA_ACTCMPAD_ZERO | PWM_3_GENA_ACTLOAD_ONE;

    PWM1_3_LOAD_R = 256;                            // set period to 40 MHz sys clock / 2 / 400 = 50 kHz
    PWM1_INVERT_R = PWM_INVERT_PWM6INV;

    PWM1_3_CMPA_R = 0;                               // blue off

    PWM1_3_CTL_R = PWM_1_CTL_ENABLE;                 // turn-on PWM0 generator 2
    PWM1_ENABLE_R = PWM_ENABLE_PWM6EN;
}

void initHw()
{
// Configure HW to work with 16 MHz XTAL, PLL enabled, sysdivider of 4, creating system clock of 40 MHz
    SYSCTL_RCC_R = SYSCTL_RCC_XTAL_16MHZ | SYSCTL_RCC_OSCSRC_MAIN
            | SYSCTL_RCC_USESYSDIV | (4 << SYSCTL_RCC_SYSDIV_S);

// Set GPIO ports to use APB (not needed since default configuration -- for clarity)
    SYSCTL_GPIOHBCTL_R = 0;

// Enable clocks
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R5 | SYSCTL_RCGCGPIO_R2;
    _delay_cycles(3);
// Configure Ports
//Outputs
    GPIO_PORTC_DIR_R |= PC7_MASK;
    GPIO_PORTF_DIR_R |= GREEN_LED_MASK | RED_LED_MASK; //Bits 1,2, and 3 are outputs
//Inputs
//Setting Drive Strength
    GPIO_PORTF_DR2R_R |= GREEN_LED_MASK | RED_LED_MASK; // set drive strength to 2mA (not needed since default configuration -- for clarity)
//Digital Enable
    GPIO_PORTC_DEN_R |= PC7_MASK;
    GPIO_PORTF_DEN_R |= GREEN_LED_MASK | RED_LED_MASK;


}


void Commands(USER_DATA data)
{   char str[MAX_CHARS+1];
    int32_t value;
    uint16_t addr = 0, index;
    static uint8_t count = 0;

    if (isCommand(&data, "clear", 0))
    {
        for (index = 0; index <= MAX_ADDR; index++)
        {
            DMX_data[index] = 0;
        }
        displayUart0("\tAll Addresses Cleared");
        displayUart0("\r\n");
        valid = true;
    }
    else if (isCommand(&data, "on", 0))
    {
        if (run)
        {
            displayUart0("\tDmx is already on!\r\n\n");
        }
        if (!run)
        {
            start_DMXTX();
            run = true;
            displayUart0("\tDMX on\r\n");
            displayUart0("\tTransmitting...\r\n");
        }
        valid = true;
    }
    else if (isCommand(&data, "off", 0))
    {
        run = false;
        DE = 0;
        displayUart0("\tDMX off\r\n");
        valid = true;
    }
    else if (isCommand(&data, "poll", 0))
    {
        displayUart0("\tPolling...\r\n");
        start_Polling();
        valid = true;
    }
    else if (isCommand(&data, "get", 1))
    {
        addr = getFieldValue(&data, 1);
        value = DMX_data[addr];
        sprintf(str,"\tValue at Address %lu is %u\r\n", addr, value);
        displayUart0(str);
        valid = true;
    }
    else if (isCommand(&data, "max", 1))
    {
        max_addr = getFieldValue(&data, 1);
        displayUart0("\tMaximum Address is: ");
        displayUart0(&data.buffer[data.Index[1]]);
        displayUart0("\r\n");
        valid = true;
    }
    else if (isCommand(&data, "set", 2))
    {
        value = getFieldValue(&data, 2);
        addr = getFieldValue(&data, 1);
        DMX_data[addr] = value;
        displayUart0("\tAddress ");
        displayUart0(&data.buffer[data.Index[1]]);
        displayUart0(" was set to ");
        displayUart0(&data.buffer[data.Index[2]]);
        displayUart0("\r\n\n");
        valid = true;
    }
    else if (isCommand(&data, "ramp", 4))
    {
        Addr[count]= getFieldValue(&data, 1); // Store Address
        DMX_data[Addr[count]] = getFieldValue(&data,2); //Set initial value
        init_val[count] = getFieldValue(&data,2); //Store Initial value
        final_val[count] = getFieldValue(&data, 3); // Store Final Value
        signal[count] = 'R'; // Store Signal Type
        time_N[count] = (getFieldValue(&data, 4))/10e-3; // Convert time and store as N interrupts
        count = (count+1)%10; //Increase row number for next entry. Limit is 10
        valid = true;
    }
    else if (isCommand(&data, "pulse", 4))
    {
        Addr[count]= getFieldValue(&data, 1); // Store Address
        DMX_data[Addr[count]] = getFieldValue(&data,2); //Set initial value
        init_val[count] = getFieldValue(&data,2); //Store Initial value
        final_val[count] = getFieldValue(&data, 3); // Store Final Value
        signal[count] = 'P'; // Store Signal Type
        time_N[count] = (getFieldValue(&data, 4))/10e-3; // Convert time and store as N interrupts
        count = (count+1)%10; //Increase row number for next entry. Limit is 10
        valid = true;
    }
    else if (isCommand(&data, "identify",0))
    {
        GREEN_LED = 1;
        RED_LED = 0;
        LED_Timeout = 50;
        valid = true;
    }
}

void System_init(void) //Checks previous Mode and Address
{
    Mode = readEeprom(MODE_ADDR);
    switch(Mode)
    {
    default: //
        Mode = Device;
    case Device:
        GPIO_PORTF_AFSEL_R |= BLUE_LED_MASK;
        Device_Addr = readEeprom(DEVICE_ADDR);
        run = false;
        start_DMXRX();
        break;
    case Controller:
        GPIO_PORTF_AFSEL_R &= ~BLUE_LED_MASK; //Use as regular LED
        run = true;
        start_DMXTX();
        break;
    }
}

void main(void)
{
    USER_DATA data;
    char str[MAX_CHARS+1];
    initHw();
    initPWM();
    initTimer();
    initUart0();
    initUart1();
    initEeprom();
    setUart0ISR();
    putsUart0("\r\n\n======================================================\r\n");
    putsUart0("---------------- EE 5314 FA2020 Project --------------\r\n");
    putsUart0("======================================================\r\n");
    blinkLED(); // Visual Indicator on board for program start
    System_init(); // Reads previous mode in EEPROM
    TIMER1_CTL_R |= TIMER_CTL_TAEN; //Start Status timer
    TIMER2_CTL_R |= TIMER_CTL_TAEN; //Start Pulse and Ramp Timer

    int x = 0;
    while (1)
    {

        for (x = 0; x < 32767; x++)
        {
            PWM1_3_CMPA_R = x;
        }

        switch (Mode)
        {

        case Device:
            if (poll_mode == false)
            {
                displayUart0("Current Mode: Device ");
                sprintf(str, "%lu\r\n", Device_Addr);
                displayUart0(str);
                displayUart0("Type in Command: ");
                getsUart0(data.buffer);
                parse(&data);
                if (isCommand(&data, "device", 1))
                {
                    Device_Addr = getFieldValue(&data, 1);
                    writeEeprom(DEVICE_ADDR, Device_Addr);
                    if (run == true)
                    {
                        run = false;
                        start_DMXRX();
                    }
                    valid = true;
                }
                else if (isCommand(&data, "controller", 0))
                {
                    Mode = Controller;
                    writeEeprom(MODE_ADDR, Mode);
                    GPIO_PORTF_AFSEL_R &= ~BLUE_LED_MASK;
                    valid = true;
                }
                else if (isCommand(&data, "identify",0))
                {
                    GREEN_LED = 1;
                    LED_Timeout = 50;
                    valid = true;
                }

                if (valid)
                {
                    data.argCount = 0;
                    valid = false;
                }
                else
                {
                    displayUart0("ERROR! Unrecognized Command\r\n\n");
                }
            }
            break;
        case Controller:
            if(poll_mode == false)
            {
                displayUart0("Current Mode: Controller\r\n");
                displayUart0("Type in Command: ");
                getsUart0(data.buffer);
                parse(&data);
                if (isCommand(&data, "device", 1))
                {
                    GPIO_PORTF_AFSEL_R |= BLUE_LED_MASK;
                    RED_LED = 0;
                    Mode = Device;
                    Device_Addr = getFieldValue(&data, 1);
                    writeEeprom(MODE_ADDR, Mode);
                    writeEeprom(DEVICE_ADDR, Device_Addr);
                    start_DMXRX();
                    valid = true;
                }
                else if (isCommand(&data, "controller", 0))
                {
                    valid = true;
                }
                Commands(data);
                if (valid)
                {
                    data.argCount = 0;
                    valid = false;
                }
                else
                {
                    displayUart0("ERROR! Unrecognized Command\r\n\n");
                }
            }
        }
    }
}
