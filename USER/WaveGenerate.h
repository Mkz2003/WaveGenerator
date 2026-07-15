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
    SINE = 0,
    SQUARE,
    TRIANGLE,
    SAWTOOTH
} Wave_t;

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions prototypes ---------------------------------------------*/
void DAC_ConfigChannel(Wave_t wave1, float freq1, float phase_deg1, Wave_t wave2, float freq2, float phase_deg2);

/* Private defines -----------------------------------------------------------*/

#endif /* __WAVEGENERATE_H */