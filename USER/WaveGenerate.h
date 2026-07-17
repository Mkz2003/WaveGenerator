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
} Wave_t;

/**
  * @brief  波形参数定义
  */
typedef struct
{
  Wave_t wave;
  float freq;
  float Vrms;
  float phase_deg;
} DAC_ChannalConfig_t;

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions prototypes ---------------------------------------------*/
void DAC_ConfigChannel(DAC_ChannalConfig_t dac_ch1, DAC_ChannalConfig_t dac_ch2, float Vdda, uint8_t waveConfig);

/* Private defines -----------------------------------------------------------*/

#endif /* __WAVEGENERATE_H */