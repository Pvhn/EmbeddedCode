#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "tm4c123gh6pm.h"

#define RED_LED     (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 1*4)))
#define BLUE_LED    (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 2*4)))
#define GREEN_LED   (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 3*4)))


// PortA masks
#define UART_TX_MASK 2
#define UART_RX_MASK 1
#define GREEN_LED_MASK 8
#define RED_LED_MASK 2
#define BLUE_LED_MASK 4
#define max_chars 80

//Non-printable characters (Used in getsUart)
#define BCKSPACE 8
#define DEL 127

void initUart0()
{
    // Enable clocks
    SYSCTL_RCGCUART_R |= SYSCTL_RCGCUART_R0;
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R0;
    _delay_cycles(3);

    // Configure UART0 pins
    GPIO_PORTA_DIR_R |= UART_TX_MASK;                   // enable output on UART0 TX pin
    GPIO_PORTA_DIR_R &= ~UART_RX_MASK;                  // enable input on UART0 RX pin
    GPIO_PORTA_DR2R_R |= UART_TX_MASK;                  // set drive strength to 2mA (not needed since default configuration -- for clarity)
    GPIO_PORTA_DEN_R |= UART_TX_MASK | UART_RX_MASK;    // enable digital on UART0 pins
    GPIO_PORTA_AFSEL_R |= UART_TX_MASK | UART_RX_MASK;  // use peripheral to drive PA0, PA1
    GPIO_PORTA_PCTL_R &= ~(GPIO_PCTL_PA1_M | GPIO_PCTL_PA0_M); // clear bits 0-7
    GPIO_PORTA_PCTL_R |= GPIO_PCTL_PA1_U0TX | GPIO_PCTL_PA0_U0RX;
                                                        // select UART0 to drive pins PA0 and PA1: default, added for clarity

    // Configure UART0 to  baud, 8N1 format
    UART0_CTL_R = 0;                                    // turn-off UART0 to allow safe programming
    UART0_CC_R = UART_CC_CS_SYSCLK;                     // use system clock (33.33 MHz)
    UART0_IBRD_R = 217;                                  // r = 33.33 MHz / (Nx9.6kHz), set floor(r)=217, where N=16
    UART0_FBRD_R = 0;                                  // round(fract(r)*64)=45
    UART0_LCRH_R = UART_LCRH_WLEN_8 | UART_LCRH_FEN;    // configure for 8N1 w/ 16-level FIFO
    UART0_CTL_R = UART_CTL_TXE | UART_CTL_RXE | UART_CTL_UARTEN;
                                                        // enable TX, RX, and module
}

void putcUart0(char c)
{
    while (UART0_FR_R & UART_FR_TXFF);               // wait if uart0 tx fifo full
    UART0_DR_R = c;                                  // write character to fifo
}

void putsUart0(char* str)
{
    uint8_t i = 0;
    while (str[i] != '\0')
        putcUart0(str[i++]);
}

char getcUart0()
{
    while (UART0_FR_R & UART_FR_RXFE);               // wait if uart0 rx fifo empty
    return UART0_DR_R & 0xFF;                        // get character from fifo
}

void getsUart0(char *str_Buffer)
{
    uint8_t count = 0;
    char c;
    while(1)
    {
        //c = '\0';
        c = getcUart0();
        putcUart0(c);
        if(c == DEL || c == BCKSPACE & count >0)
        {
            count--;
            continue;
        }
        else if(c == DEL || c == BCKSPACE & count == 0)
        {
            continue;
        }
        else
        {
            if(c == '\r' )
            {
                str_Buffer[count] = '\0';
                putcUart0('\n');
                break;
            }
            else if(c >= ' ')
            {
                str_Buffer[count++] = c;
                if(count == max_chars)
                {
                    str_Buffer[count] = '\0';
                    putcUart0('\n');
                    break;
                }
            }
        }
    }
}

void initHw()
{
    // Configure HW to work with 16 MHz XTAL, PLL enabled, sysdivider of 5, creating system clock of 33.3 MHz
    SYSCTL_RCC_R = SYSCTL_RCC_XTAL_16MHZ | SYSCTL_RCC_OSCSRC_MAIN | SYSCTL_RCC_USESYSDIV | (5 << SYSCTL_RCC_SYSDIV_S);

    // Set GPIO ports to use APB (not needed since default configuration -- for clarity)
    SYSCTL_GPIOHBCTL_R = 0;

    // Enable clocks
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R5;
    _delay_cycles(3);
    // Configure Ports
    //Outputs
    GPIO_PORTF_DIR_R |= GREEN_LED_MASK | RED_LED_MASK |BLUE_LED_MASK; //Bits 1,2, and 3 are outputs
    //Inputs
    //Setting Drive Strength
    GPIO_PORTF_DR2R_R |= GREEN_LED_MASK | RED_LED_MASK |BLUE_LED_MASK;  // set drive strength to 2mA (not needed since default configuration -- for clarity)
    //Digital Enable
    GPIO_PORTF_DEN_R |= GREEN_LED_MASK | RED_LED_MASK | BLUE_LED_MASK;
}

//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------

int main(void)
{
    // Initialize hardware
    initUart0();
    initHw();
    char input[max_chars+1];
    while(true)
    {
        getsUart0(input);
        //putsUart0(input);

        if(strcmp(input, "ON")==0 || strcmp(input, "on")==0) //Strcmp returns 0 if both are equal
        {
            GREEN_LED = 1;
            //putsUart0("GREEN LED ON\n");
        }
        else
        {
            GREEN_LED = 0;
            //putsUart0("LED Off\n");
        }
    }
}
