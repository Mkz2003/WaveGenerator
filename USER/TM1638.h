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
void TM1638_Display(uint16_t data[8]);
void TM1638_WriteSegments(uint8_t seg, uint8_t grid, uint16_t data[8]);
void TM1638_FloatToSegments(float value, uint16_t data[8]);

/* Private defines -----------------------------------------------------------*/

#endif /* __TM1638_H */
