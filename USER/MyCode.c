#include "MyCode.h"

#include <stdio.h>

#include "Date.h"
#include "WaveGenerate.h"

uint8_t buf[1024];

void Setup(void)
{
    HAL_UARTEx_ReceiveToIdle_DMA(&huart2, buf, sizeof(buf));

    RTCTimeInit();




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
        printf("%d-%d-%d %d:%d:%d.%03ld\r\n", sDate.Year, sDate.Month, sDate.Date, sTime.Hours, sTime.Minutes, sTime.Seconds, 1000 - sTime.SubSeconds * 1000 / (sTime.SecondFraction + 1));
        HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);

        LEDTime = t;
    }      

    static uint32_t DACTime = 0;
    if((t = HAL_GetTick()) - DACTime >= 1000)
    {
        static int i = 0;
        switch(i++)
        {
            case 0:
            {
                DAC_ConfigChannel(SINE, 1000, 0, SQUARE, 1000, 0);
                break;
            }
            case 1:
            {
                DAC_ConfigChannel(SINE, 1000, 0, TRIANGLE, 1000, 0);
                break;
            }
            case 2:
            {
                DAC_ConfigChannel(SAWTOOTH, 1000, 0, TRIANGLE, 1000, 0);
                break;
            }
            case 3:
            {
                DAC_ConfigChannel(SAWTOOTH, 1000, 0, SINE, 1000, 0);
                break;
            }
            case 4:
            {
                DAC_ConfigChannel(SAWTOOTH, 2000, 0, SINE, 2000, 0);
                break;
            }
            case 5:
            {
                DAC_ConfigChannel(SQUARE, 2000, 0, SINE, 2000, 0);
                break;
            }
            case 6:
            {
                DAC_ConfigChannel(TRIANGLE, 2000, 0, SQUARE, 2000, 0);
                break;
            }
            case 7:
            {
                DAC_ConfigChannel(SINE, 2000, 0, SQUARE, 2000, 0);
                break;
            }
        }
        if(i >= 8) i = 0;
        DACTime = t;
    }    


}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart->Instance == USART2)
    {
        HAL_UARTEx_ReceiveToIdle_DMA(&huart2, buf, sizeof(buf));
    }
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if(huart->Instance == USART2)
    {
        HAL_UART_Transmit_DMA(&huart2, buf, Size);
    }
}
