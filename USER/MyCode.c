#include "MyCode.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "Date.h"
#include "WaveGenerate.h"
#include "TM1638.h"

uint8_t buf[1024];

double freq = 1.0;
Wave_t wave1 = SINE, wave2 = TRIANGLE;

void Setup(void)
{
    HAL_UARTEx_ReceiveToIdle_DMA(&huart2, buf, sizeof(buf) - 1);

    RTCTimeInit();

    DAC_ConfigChannel(wave1, freq, 0, wave2, freq, 0);


}

void Loop(void)
{
    RTC_DateTypeDef sDate;
    RTC_TimeTypeDef sTime;

    uint32_t t;
    static uint32_t LEDTime = 0;
    if((t = HAL_GetTick()) - LEDTime >= 1000)
    {
        HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
        HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);  
        printf("%d-%d-%d %d:%d:%d.%03ld\n", sDate.Year, sDate.Month, sDate.Date, sTime.Hours, sTime.Minutes, sTime.Seconds, 1000 - sTime.SubSeconds * 1000 / (sTime.SecondFraction + 1));
        // HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);

        LEDTime = t;
    }      

    uint32_t k = TM1638_ReadKeys();
    uint8_t data[16] = {0};
    DoubleToSegments(k, data);
    for(int i = 0; i < 8; i++)
    {
        data[i * 2 + 1] = 0xFF & !!(k & (0x1 << (i * 4)));
    }
    TM1638_DisplayDigits(data);
}

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
