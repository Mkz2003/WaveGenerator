/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __TM1638_H
#define __TM1638_H

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* Exported types ------------------------------------------------------------*/
/**
  * @brief  按键状态枚举值定义
  */
typedef enum
{
    KEY_RELEASE = 0,
    KEY_CLICK,
    KEY_LONGPRESS,
    KEY_NONE
} KeyStatus_t;

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions prototypes ---------------------------------------------*/
void TM1638_DisplayBrightness(uint8_t brightness);
uint32_t TM1638_ReadKeys(void);
KeyStatus_t TM1638_KeyStatus(uint8_t k, uint8_t ks);
void TM1638_DisplayDigits(uint8_t data[16]);
void FloatToSegments(float value, uint8_t data[16]);

/* Private defines -----------------------------------------------------------*/

#endif /* __TM1638_H */
