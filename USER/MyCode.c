#include "MyCode.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "Date.h"
#include "WaveGenerate.h"

uint8_t buf[1024];

volatile int setting = 0;
volatile double freq = 1.0;
Wave_t wave1 = SINE, wave2 = TRIANGLE;

void HAL_GPIO_EXTI_Rising_Callback(uint16_t GPIO_Pin)
{
    if(GPIO_Pin == GPIO_PIN_0)
    {
        HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, 1);
        setting = 1;
    }
}

void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin)
{
    if(GPIO_Pin == GPIO_PIN_0)
    {
        HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, 0);
    }    
}


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

    if(setting)
    {

        // if(freq < 100000.0)
        // {
        //     // freq *= 10.0;
        // }
        // else
        // {
        //     freq = 1.0;
            wave1 = wave1 != SINE ? SINE : SAWTOOTH;
            wave2 = wave2 != TRIANGLE ? TRIANGLE : SQUARE;
        // }
        DAC_ConfigChannel(wave1, freq, 0, wave2, freq, 0);
        setting = 0;
    }

    // static uint32_t DACTime = 0;
    // if((t = HAL_GetTick()) - DACTime >= 1000)
    // {
    //     static int i = 0;
    //     switch(i++)
    //     {
    //         case 0:
    //         {
    //             DAC_ConfigChannel(SINE, 1000, 0, SQUARE, 1000, 0);
    //             break;
    //         }
    //         case 1:
    //         {
    //             DAC_ConfigChannel(SINE, 1000, 0, TRIANGLE, 1000, 0);
    //             break;
    //         }
    //         case 2:
    //         {
    //             DAC_ConfigChannel(SAWTOOTH, 1000, 0, TRIANGLE, 1000, 0);
    //             break;
    //         }
    //         case 3:
    //         {
    //             DAC_ConfigChannel(SAWTOOTH, 1000, 0, SINE, 1000, 0);
    //             break;
    //         }
    //         case 4:
    //         {
    //             DAC_ConfigChannel(SAWTOOTH, 2000, 0, SINE, 2000, 0);
    //             break;
    //         }
    //         case 5:
    //         {
    //             DAC_ConfigChannel(SQUARE, 2000, 0, SINE, 2000, 0);
    //             break;
    //         }
    //         case 6:
    //         {
    //             DAC_ConfigChannel(TRIANGLE, 2000, 0, SQUARE, 2000, 0);
    //             break;
    //         }
    //         case 7:
    //         {
    //             DAC_ConfigChannel(SINE, 2000, 0, SQUARE, 2000, 0);
    //             break;
    //         }
    //     }
    //     if(i >= 8) i = 0;
    //     DACTime = t;
    // }    


}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart->Instance == USART2)
    {
        // HAL_UARTEx_ReceiveToIdle_DMA(&huart2, buf, sizeof(buf) - 1);
    }
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if(huart->Instance == USART2)
    {
        if(isdigit(buf[0]))
        {
            double f = atof((const char*)buf);
            if(f > 0.0) freq = f;
        }
        DAC_ConfigChannel(wave1, freq, 0, wave2, freq, 0);
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
