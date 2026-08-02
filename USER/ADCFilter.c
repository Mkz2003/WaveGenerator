#include "ADCFilter.h"

#include <arm_math.h>

void ADCFilter_Init(ADCFilter_t* adcFilter, float32_t sampleFreq, float32_t cutoffFreq)
{
    float32_t coeffs[5];

    // 设计二阶Butterworth低通滤波器
	float32_t omega = 2.0f * PI * cutoffFreq / sampleFreq;
	float32_t sn = arm_sin_f32(omega);
	float32_t cs = arm_cos_f32(omega);
	float32_t alpha = sn / (2.0f * 0.7071f); // Butterworth的Q因子为0.707
	
	float32_t b0 = (1.0f - cs) / 2.0f;
	float32_t b1 = 1.0f - cs;
	float32_t b2 = b0;
	float32_t a0 = 1.0f + alpha;
	float32_t a1 = -2.0f * cs;
	float32_t a2 = 1.0f - alpha;

    // 归一化系数
    coeffs[0] = b0 / a0; // b0
    coeffs[1] = b1 / a0; // b1
    coeffs[2] = b2 / a0; // b2
    coeffs[3] = -(a1 / a0); // a1
    coeffs[4] = -(a2 / a0); // a2

    memset(adcFilter->state, 0, sizeof(adcFilter->state));
    memcpy(adcFilter->coeffs, coeffs, sizeof(adcFilter->coeffs));
    arm_biquad_cascade_df1_init_f32(&adcFilter->S, 1, adcFilter->coeffs, adcFilter->state);
}

float32_t ADCFilter(ADCFilter_t* adcFilter, float32_t input)
{
    float32_t output;
    if(adcFilter->init != 0)
    {
        arm_biquad_cascade_df1_f32(&adcFilter->S, &input, &output, 1);
    }
    else 
    {
        for(int i = 0; i < 4; i++) adcFilter->state[i] = input;
        output = input;
        adcFilter->init = 1;
    }
    return output;
}


