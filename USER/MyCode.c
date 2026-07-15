/* Includes ------------------------------------------------------------------*/
#include "MyCode.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "main.h"

/* Private includes ----------------------------------------------------------*/
#include "Date.h"
#include "WaveGenerate.h"
#include "TM1638.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
#define TASK_INIT() typeof(HAL_GetTick()) __task_tick;

#define TASK_START(TASK, TIME)                          \
    static uint32_t TASK = 0;                           \
    if((__task_tick = HAL_GetTick()) - TASK >= TIME)    \
    {

#define TASK_END(TASK)                                  \
        TASK = __task_tick;                             \
    }
        
/* Private variables ---------------------------------------------------------*/
uint8_t buf[1024];

float freq = 1.0f;
Wave_t wave1 = SINE, wave2 = TRIANGLE;

volatile struct {volatile uint16_t Temp, Vref, Vbat;} adcval = {0};

/* Private function prototypes -----------------------------------------------*/
/* Exported Constants --------------------------------------------------------*/
extern RTC_HandleTypeDef hrtc;
extern UART_HandleTypeDef huart2;
extern ADC_HandleTypeDef hadc1;

/* Exported functions --------------------------------------------------------*/
/**
  * @brief      main函数的初始化过程
  * @retval     none
  */
void Setup(void)
{
    HAL_UARTEx_ReceiveToIdle_DMA(&huart2, buf, sizeof(buf) - 1);

    RTCTimeInit();

    DAC_ConfigChannel(wave1, freq, 0, wave2, freq, 0);


    HAL_ADCEx_Calibration_Start(&hadc1);
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)&adcval, sizeof(adcval) / sizeof(uint16_t));
}

/**
  * @brief      main函数的循环过程
  * @retval     none
  */
void Loop(void)
{
    static float Vdda = 0.0f, Vbat = 0.0f, Temp = 0.0f; 
    static RTC_DateTypeDef sDate;
    static RTC_TimeTypeDef sTime;

    TASK_INIT()

    TASK_START(LED, 1000)
    {
        HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
        HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

        const uint8_t fractional_precision = 4;
        const float pow10fp = pow(10.0f, fractional_precision);

        float vrefint_cal_vref = (VREFINT_CAL_VREF / 1000.0f);
        float temperature_cal1_temp = TEMPSENSOR_CAL1_TEMP;
        float temperature_cal2_temp = TEMPSENSOR_CAL2_TEMP;
        float vrefint_cal_addr = (*VREFINT_CAL_ADDR) << 4;
        float tempsensor_cal1_addr = (*TEMPSENSOR_CAL1_ADDR) << 4;
        float tempsensor_cal2_addr = (*TEMPSENSOR_CAL2_ADDR) << 4;
        Vdda = vrefint_cal_vref * vrefint_cal_addr / adcval.Vref;
        Vbat = adcval.Vbat * Vdda / (float)(1 << 16) * 3.0f;
        // Temp = t1 + ((ADC) - adc1) * (t2 - t1) / (adc2 - adc1)
        Temp = temperature_cal1_temp
                    + (adcval.Temp / vrefint_cal_vref * Vdda - tempsensor_cal1_addr)
                        * (temperature_cal2_temp - temperature_cal1_temp)
                        / (tempsensor_cal2_addr - tempsensor_cal1_addr);

        int Vdda_t1 = truncf(Vdda), Vdda_t100 = truncf(Vdda * pow10fp) - Vdda_t1 * pow10fp;
        int Vbat_t1 = truncf(Vbat), Vbat_t100 = truncf(Vbat * pow10fp) - Vbat_t1 * pow10fp;
        int Temp_t1 = truncf(Temp), Temp_t100 = truncf(Temp * pow10fp) - Temp_t1 * pow10fp;
        printf("%2d-%2d-%2d %2d:%02d:%02d.%03ld\n"
                "Vdda: %d.%0*d, Vbat: %d.%0*d, Temp: %d.%0*d\n"
                "\n",
                sDate.Year, sDate.Month, sDate.Date, sTime.Hours, sTime.Minutes, sTime.Seconds, 1000 - sTime.SubSeconds * 1000 / (sTime.SecondFraction + 1),
                Vdda_t1, fractional_precision, Vdda_t100, Vbat_t1, fractional_precision, Vbat_t100, Temp_t1, fractional_precision, Temp_t100
                );
        // HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);

    }
    TASK_END(LED)

    TASK_START(ADCSAMPLING, 100)
    {

    }
    TASK_END(ADCSAMPLING)

    TASK_START(WAVECTRL, 20)
    {
        uint32_t keyValue = TM1638_ReadKeys();

        if(TM1638_KeyStatus(3, 1) == KEY_CLICK || TM1638_KeyStatus(3, 1) == KEY_LONGPRESS)
        {
            freq += 1.0f;
        }
        if(TM1638_KeyStatus(3, 3) == KEY_CLICK || TM1638_KeyStatus(3, 3) == KEY_LONGPRESS)
        {
            freq -= 1.0f;
        }


        
        uint8_t data[16] = {0};
        FloatToSegments(freq, data);
        // DoubleToSegments(sTime.Hours * 10000 + sTime.Minutes * 100 + sTime.Seconds + (1000 - sTime.SubSeconds * 1000 / (sTime.SecondFraction + 1)) * 0.001, data);
        for(int i = 0; i < 8; i++)
        {
            data[i * 2 + 1] = TM1638_KeyStatus(3, i + 1) == KEY_LONGPRESS ? 0xFF : 0xFE;
        }
        TM1638_DisplayDigits(data);
    }
    TASK_END(WAVECTRL)
}
