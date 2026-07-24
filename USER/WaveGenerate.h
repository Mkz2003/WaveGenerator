/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __WAVEGENERATE_H
#define __WAVEGENERATE_H

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* Exported types ------------------------------------------------------------*/
/**
  * @brief  波形枚举值定义
  */
typedef enum
{
    DC = 0,
    SINE,
    SQUARE,
    TRIANGLE,
    SAWTOOTH
} WaveForm_t;

/**
  * @brief  波形参数定义
  */
typedef struct
{
  WaveForm_t waveForm;
  float freq;
  float Vrms;
  float phase_deg;
} Wave_t;

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions prototypes ---------------------------------------------*/
void Set_Wave(const Wave_t* const wave, float Vdda, uint8_t waveConfig);

void DAC_ConvHalfCpltCallbackCh1(void);
void DAC_ConvCpltCallbackCh1(void);

/* Private defines -----------------------------------------------------------*/

#endif /* __WAVEGENERATE_H */