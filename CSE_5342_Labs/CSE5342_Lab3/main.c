/*
 * main.c
 *
 *Created on: Sep 7, 2024
 *Author: peter
 */

#include "Utilities.h"

int main(void)
{
    System_Init();
    Key_Init();
    DAC_Init();
    Sound_Init();

    uint32_t keys_pv = 0;

    while(1)
    {
        if(keys == 0)
        {
            play = 0;
        }
        else
        {
            Sound_Play(Notes[keys-1]);
        }

//        Sound_Play(Notes[3]);
//        _delay_cycles(3000000);
//        Sound_Play(Notes[5]);
//        _delay_cycles(3000000);
//        Sound_Play(Notes[8]);
//        _delay_cycles(3000000);
//        Sound_Play(Notes[5]);
//        _delay_cycles(3000000);
//        Sound_Play(Notes[12]);
//        _delay_cycles(4000000);
//        play = 0;
//        _delay_cycles(4000000);
//        Sound_Play(Notes[12]);
//        _delay_cycles(4000000);
//        Sound_Play(Notes[10]);
//        _delay_cycles(7000000);
//        play = 0;
//        _delay_cycles(7000000);
//        Sound_Play(Notes[3]);
//        _delay_cycles(3000000);
//        Sound_Play(Notes[5]);
//        _delay_cycles(3000000);
//        Sound_Play(Notes[7]);
//        _delay_cycles(3000000);
//        Sound_Play(Notes[3]);
//        _delay_cycles(3000000);
//        Sound_Play(Notes[10]);
//        _delay_cycles(4000000);
//        play = 0;
//        _delay_cycles(4000000);
//        Sound_Play(Notes[10]);
//        _delay_cycles(5000000);
//        Sound_Play(Notes[8]);
//        _delay_cycles(6000000);
//        play = 0;
//        _delay_cycles(7000000);
//        Sound_Play(Notes[3]);
//        _delay_cycles(3000000);
//        Sound_Play(Notes[5]);
//        _delay_cycles(3000000);
//        Sound_Play(Notes[8]);
//        _delay_cycles(3000000);
//        Sound_Play(Notes[5]);
//        _delay_cycles(3000000);
//        Sound_Play(Notes[8]);
//        _delay_cycles(7000000);
//        Sound_Play(Notes[10]);
//        _delay_cycles(3000000);
//        Sound_Play(Notes[7]);
//        _delay_cycles(5000000);
//        Sound_Play(Notes[5]);
//        _delay_cycles(4000000);
//        Sound_Play(Notes[3]);
//        _delay_cycles(7000000);
//        play = 0;
//        _delay_cycles(7000000);
//        Sound_Play(Notes[3]);
//        _delay_cycles(5000000);
//        Sound_Play(Notes[10]);
//        _delay_cycles(8000000);
//        Sound_Play(Notes[8]);
//        _delay_cycles(7000000);
//        play = 0;
//        _delay_cycles(80000000);

    }
}
