#include "main.h"

extern UART_HandleTypeDef huart2;

#ifdef __GNUC__
int __io_putchar(int ch)
{
#else
int fputc(int ch, FILE *f)
{
#endif
    // 发送字符到串口
    HAL_UART_Transmit(&huart2, (uint8_t*)&ch, 1, HAL_MAX_DELAY);
    
    return ch;
}
