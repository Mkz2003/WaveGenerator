#ifndef __WAVEGENERATE_H
#define __WAVEGENERATE_H

#include <stdint.h>

typedef enum
{
    SINE = 0,
    SQUARE,
    TRIANGLE,
    SAWTOOTH
} Wave_t;

void DAC_ConfigChannel(Wave_t wave1, float freq1, float phase_deg1, Wave_t wave2, float freq2, float phase_deg2);

#endif // __WAVEGENERATE_H