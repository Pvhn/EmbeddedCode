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
volatile uint32_t SineWave[DAC_SIZE*2] = {0,};
volatile uint32_t play = 0;

// Array containing frequencies of certain notes.
const uint32_t Notes[16] =
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

/*=======================================================
 * Function Name: Sound_Init
 *=======================================================
 * Parameters: None
 * Returns: None
 * Description:
 * This function initializes the Sys Tick timer for use
 * for outputting the sound data. This function will also
 * populate an array with samples for a Sine Wave.
 *========================================================*/
void Sound_Init(void)
{
    uint32_t indx = 0;

    // Initialize SysTick Timer
    NVIC_ST_CTRL_R = 0; // Ensure SysTick is disabled before config
    NVIC_SYS_PRI3_R = (NVIC_SYS_PRI3_R&0x00FFFFFF) | 0x20000000; // Setting Interrupt Priority to 1.
    NVIC_ST_CTRL_R = 0x0005; // Use the Core Clock and enable SysTick.

    // One time set up of the Sine Wave. The signal is given an offset of 32 and
    // an amplitude of 32 so that the wave is output on a scale from 0 to 3.3V
    // The amplitude is minus 1 so that the full scale value does not exceed the
    // size of the DAC. (Based on a 6-bit (64 possible values) DAC)
    // The step size of each sample is 1/64. Note, there is a relationship
    // between the step size per sample and the frequency bandwidth of playable notes.
    // The smaller the step size, the smaller the frequency bandwidth.
    for (indx = 0; indx < DAC_SIZE*2; indx++)
    {
        SineWave[indx] =  (uint32_t) ((31*sin((indx*M_PI)/DAC_SIZE)) + 32);
    }

    play = 0;
    NVIC_ST_CTRL_R |= 0x0002;
}

/*=======================================================
 * Function Name: Sound_Play
 *=======================================================
 * Parameters: frequency
 * Returns: None
 * Description:
 * This function reconfigures the SysTick Timer to
 * output sound samples at the specified frequency
 * input.
 *========================================================*/
void Sound_Play(uint32_t frequency)
{
    // Calculated interrupt frequency is determined by:
    // System Clock Freq/ (Desired Freq * Scale Factor).
    // Note: The scale factor is the total number of samples in the Sine Wave
    // to be generated. For this case, it is twice the size of the DAC output
    const uint32_t ticks = (40e6)/(DAC_SIZE*2*frequency);

    // Load the SysTick Timer
    NVIC_ST_RELOAD_R = ticks - 1;

    play = 1;
}

void Sound_Stop(void)
{
    // Set SysTick to interrupt at about 1
    const uint32_t ticks = (40e6/10);

    // Load the SysTick Timer
    NVIC_ST_RELOAD_R = ticks - 1;

    play = 0;
}

void SysTickISR(void)
{
    static uint32_t sample = 0;

    HBLED ^= 1;
    if(play)
    {

        // Increment the sample and output the sample to the DAC
        sample = (sample+1) % 128;
        DAC_Out(SineWave[sample]);
    }
    else
    {
        DAC_OUT = 0;
//        NVIC_ST_CTRL_R &= ~0x0002;
    }
}
