/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __ADCFILTER_H
#define __ADCFILTER_H

/* Includes ------------------------------------------------------------------*/
#include <arm_math.h>

/* Exported types ------------------------------------------------------------*/
typedef struct
{
    arm_biquad_casd_df1_inst_f32 S;
    float32_t coeffs[5];
    float32_t state[4];
    uint8_t init;
} ADCFilter_t;

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions prototypes ---------------------------------------------*/
void ADCFilter_Init(ADCFilter_t* adcFilter, float32_t sampleFreq, float32_t cutoffFreq);
float32_t ADCFilter(ADCFilter_t* adcFilter, float32_t input);

/* Private defines -----------------------------------------------------------*/

#endif /* __ADCFILTER_H */