#include "main.h"

extern UART_HandleTypeDef huart2;

extern uint8_t buf[1024];

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart->Instance == USART2)
    {
        HAL_UARTEx_ReceiveToIdle_DMA(&huart2, buf, sizeof(buf) - 1);
    }
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if(huart->Instance == USART2)
    {
        buf[Size] = '\n';
        HAL_UART_Transmit_DMA(&huart2, buf, Size);
        HAL_UARTEx_ReceiveToIdle_DMA(&huart2, buf, sizeof(buf) - 1);
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if(huart->Instance == USART2)
    {
        HAL_UARTEx_ReceiveToIdle_DMA(&huart2, buf, sizeof(buf) - 1);
    }
}
