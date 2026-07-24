#include "main.h"

#include "MyCode.h"
#include "WaveGenerate.h"

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart->Instance == USART2)
    {
        UART_TxCpltCallback();
    }
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if(huart->Instance == USART2)
    {
        UART_RxEventCallback(Size);
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if(huart->Instance == USART2)
    {
        UART_ErrorCallback();
    }
}

/* 半传输完成回调：填充前半缓冲 */
void HAL_DAC_ConvHalfCpltCallbackCh1(DAC_HandleTypeDef *hdac)
{
    DAC_ConvHalfCpltCallbackCh1();
}

/* 传输完成回调：填充后半缓冲 */
void HAL_DAC_ConvCpltCallbackCh1(DAC_HandleTypeDef *hdac)
{
    DAC_ConvCpltCallbackCh1();
}
