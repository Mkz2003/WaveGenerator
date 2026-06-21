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

void DAC_ConfigChannel(Wave_t wave1, double freq1, double phase_deg1, Wave_t wave2, double freq2, double phase_deg2);

#endif // __WAVEGENERATE_H