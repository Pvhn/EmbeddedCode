/*
 * Sound.c
 *
 *  Created on: Sep 28, 2024
 *      Author: peter
 */



#include "Sound.h"


/*========================================================
 * Variable Definitions
 *========================================================
 */
uint32_t sample = 0;
uint32_t SineWave[DAC_SIZE*2] = {0,};
uint32_t play = 0;

uint32_t Notes[16] =
{
    1397,   // F
    1480,   // F#/Gb
    1568,   // G
    1661,   // G#/Ab
    1760,   // A
    1865,   // A#/Bb
    1975,   // B
    2093,   // C
    2217,   // C#/Db
    2349,   // D
    2489,   // D#/Eb
    2637,   // E
    2794,   // F
    2960,   // F#/Gb
    3136,   // G
    3322,   // G#/Ab
};

/*========================================================
 * Function Definitions
 *========================================================
 */

void Sound_Init(void)
{
    uint32_t indx = 0;

    // Initialize SysTick Timer
    NVIC_ST_CTRL_R = 0; // Ensure SysTick is disabled before config
    NVIC_SYS_PRI3_R = (NVIC_SYS_PRI3_R&0x00FFFFFF)|0x20000000;
    NVIC_ST_CTRL_R = 0x0005; // Use the Core Clock and enable SysTick.

    // One time set up of the Sine Wave. The signal is given an offset of 32 and
    // an amplitude of 32 so that the wave is output on a scale from 0 to 3.3V
    // (Based on a 6-bit (64 possible values) DAC)
    for (indx = 0; indx < DAC_SIZE*2; indx++)
    {
        SineWave[indx] =  (uint32_t) ((32-1)*sin((indx*M_PI)/DAC_SIZE) + 32);
    }

    // Initialize sine output to first sample;
    sample = 0;
}

void Sound_Play(uint32_t frequency)
{
    // Calculated interrupt frequency is determined by:
    // System Clock Freq/ (Desired Freq * 128).
    // Note: The 128 scale factor is dependent on the total size of the wave
    // to be generated. In our case it is twice the size of our DAC (64*2)
    uint32_t ticks = (40e6)/(128*frequency);

    play = 1;

    NVIC_ST_CTRL_R |= 0x0002;
    // Load the SysTick Timer and clear it
    NVIC_ST_RELOAD_R = ticks - 1;
}

void SysTickISR(void)
{
    if(play)
    {
        HBLED ^= 1;
        sample = (sample+1) % (128);
        DAC_Out(SineWave[sample]);
    }
    else
    {
        HBLED = 0 ;
        DAC_OUT = 0;
        NVIC_ST_CTRL_R &= ~0x0002;
    }

}
