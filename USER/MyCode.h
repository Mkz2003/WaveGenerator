/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MYCODE_H
#define __MYCODE_H

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions prototypes ---------------------------------------------*/
void Setup(void);
void Loop(void);

void UART_TxCpltCallback(void);
void UART_RxEventCallback(uint16_t Size);
void UART_ErrorCallback(void);

/* Private defines -----------------------------------------------------------*/

#endif /* __MYCODE_H */