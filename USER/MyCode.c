#include "MyCode.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "Date.h"
#include "WaveGenerate.h"
#include "TM1638.h"

#define TASK_INIT() typeof(HAL_GetTick()) __task_tick;

#define TASK_START(TASK, TIME)                          \
    static uint32_t TASK = 0;                           \
    if((__task_tick = HAL_GetTick()) - TASK >= TIME)    \
    {

#define TASK_END(TASK)                                  \
        TASK = __task_tick;                             \
    }           

uint8_t buf[1024];

float freq = 1.0f;
Wave_t wave1 = SINE, wave2 = TRIANGLE;

void Setup(void)
{
    HAL_UARTEx_ReceiveToIdle_DMA(&huart2, buf, sizeof(buf) - 1);

    RTCTimeInit();

    DAC_ConfigChannel(wave1, freq, 0, wave2, freq, 0);


}

void Loop(void)
{
    static RTC_DateTypeDef sDate;
    static RTC_TimeTypeDef sTime;

    TASK_INIT()

    TASK_START(LED, 1000)
    {
        HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
        HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);  
        printf("%d-%d-%d %d:%d:%d.%03ld\n", sDate.Year, sDate.Month, sDate.Date, sTime.Hours, sTime.Minutes, sTime.Seconds, 1000 - sTime.SubSeconds * 1000 / (sTime.SecondFraction + 1));
        HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);

    }
    TASK_END(LED)

    TASK_START(WAVECTRL, 20)
    {
        uint32_t keyValue = TM1638_ReadKeys();


        if(TM1638_KeyStatus(keyValue, 3, 1))
        {
            freq += 1.0f;
        }
        if(TM1638_KeyStatus(keyValue, 3, 3))
        {
            freq -= 1.0f;
        }


        freq += powf(10.0f, floorf(fabsf(log10f(fabsf(freq))))) / 10.0f;
        if(freq > 10000000.0f) freq = -10000000.0f;














        
        uint8_t data[16] = {0};
        DoubleToSegments(freq, data);
        // DoubleToSegments(sTime.Hours * 10000 + sTime.Minutes * 100 + sTime.Seconds + (1000 - sTime.SubSeconds * 1000 / (sTime.SecondFraction + 1)) * 0.001, data);
        for(int i = 0; i < 8; i++)
        {
            data[i * 2 + 1] = TM1638_KeyStatus(keyValue, 3, i + 1) ? 0xFF : 0xFE;
        }
        TM1638_DisplayDigits(data);
    }
    TASK_END(WAVECTRL)
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
