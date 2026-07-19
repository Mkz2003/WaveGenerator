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
const uint32_t VrmsRemainTime = 2000;

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

uint32_t freq100 = 1000 * 100;
uint32_t Vrms100 = 1 * 100;
Wave_t wave1 = SINE, wave2 = DC;

volatile struct {volatile uint16_t Temp, Vref, Vbat;} adcval = {0};

/* Private function prototypes -----------------------------------------------*/
static uint32_t Int100Digits(uint32_t v100);

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

    DAC_ChannalConfig_t dac_ch1 = {wave1, freq100 / 100.0f, Vrms100 / 100.0f, 0.0f};
    DAC_ChannalConfig_t dac_ch2 = {wave2, freq100 / 100.0f, Vrms100 / 100.0f, 0.0f};

    DAC_ConfigChannel(dac_ch1, dac_ch2, 3.3f, 1);

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

    TASK_START(ADCSAMPLING, 1)
    {
        float vrefint_cal_vref = (VREFINT_CAL_VREF / 1000.0);
        float temperature_cal1_temp = TEMPSENSOR_CAL1_TEMP;
        float temperature_cal2_temp = TEMPSENSOR_CAL2_TEMP;
        float vrefint_cal_addr = (*VREFINT_CAL_ADDR) << 4;
        float tempsensor_cal1_addr = (*TEMPSENSOR_CAL1_ADDR) << 4;
        float tempsensor_cal2_addr = (*TEMPSENSOR_CAL2_ADDR) << 4;
        Vdda = vrefint_cal_vref * vrefint_cal_addr / adcval.Vref;
        Vbat = adcval.Vbat * Vdda / (float)(1 << 16) * 3.0;
        // Temp = t1 + ((ADC) - adc1) * (t2 - t1) / (adc2 - adc1)
        Temp = temperature_cal1_temp
                    + (adcval.Temp / vrefint_cal_vref * Vdda - tempsensor_cal1_addr)
                        * (temperature_cal2_temp - temperature_cal1_temp)
                        / (tempsensor_cal2_addr - tempsensor_cal1_addr);
    }
    TASK_END(ADCSAMPLING)

    TASK_START(WAVECTRL, 20)
    {

        const uint32_t freq100Max = 10000000, freq100Min = 10;
        const uint32_t Vrms100Max = 100, Vrms100Min = 1;

        // 从TM1638读按键
        uint32_t keyValue = TM1638_ReadKeys();

        uint8_t waveConfig = 0;
        static uint32_t VrmsSelectTick = 0x7FFFFFFF;

        // 频率+
        if(TM1638_KeyStatus(3, 1) == KEY_CLICK || TM1638_KeyStatus(3, 1) == KEY_LONGPRESS)
        {
            freq100 += Int100Digits(freq100);
            if(freq100 > freq100Max) freq100 = freq100Max;
            waveConfig = 1;
            VrmsSelectTick -= VrmsRemainTime;   // 不让显示Vrms以显示Freq
        }

        // 频率-
        if(TM1638_KeyStatus(3, 3) == KEY_CLICK || TM1638_KeyStatus(3, 3) == KEY_LONGPRESS)
        {
            freq100 -= Int100Digits(freq100 - Int100Digits(freq100));
            if(freq100 < freq100Min) freq100 = freq100Min;
            waveConfig = 1;
            VrmsSelectTick -= VrmsRemainTime;   // 不让显示Vrms以显示Freq
        }

        // 电压+
        if(TM1638_KeyStatus(3, 5) == KEY_CLICK || TM1638_KeyStatus(3, 5) == KEY_LONGPRESS)
        {
            Vrms100 += Int100Digits(Vrms100);
            if(Vrms100 > Vrms100Max) Vrms100 = Vrms100Max;
            waveConfig = 1;
            VrmsSelectTick = HAL_GetTick();
        }

        // 电压-
        if(TM1638_KeyStatus(3, 7) == KEY_CLICK || TM1638_KeyStatus(3, 7) == KEY_LONGPRESS)
        {
            Vrms100 -= Int100Digits(Vrms100 - Int100Digits(Vrms100));
            if(Vrms100 < Vrms100Min) Vrms100 = Vrms100Min;
            waveConfig = 1;
            VrmsSelectTick = HAL_GetTick();
        }

        // 波形切换
        static uint8_t waveSelect_entryFlag = 1;    // 令按键单击和长按只触发一次事件
        if(TM1638_KeyStatus(3, 2) == KEY_CLICK || TM1638_KeyStatus(3, 2) == KEY_LONGPRESS)
        {
            if(waveSelect_entryFlag != 0)
            {
                Wave_t* wave = &wave1;
                switch(*wave)
                {
                    case SINE:
                    {
                        *wave = TRIANGLE;
                        break;
                    }
                    case TRIANGLE:
                    {
                        *wave = SQUARE;
                        break;
                    }
                    case SQUARE:
                    {
                        *wave = SAWTOOTH;
                        break;
                    }
                    case SAWTOOTH:
                    default:
                    {
                        *wave = SINE;
                        break;
                    }
                }
                waveConfig = 1;
                waveSelect_entryFlag = 0;
            }
        }
        else
        {
            waveSelect_entryFlag = 1;
        }

        // 启动DAC配置
        if(waveConfig != 0 || 1)    // 使能DAC的幅值动态调整
        {
            DAC_ChannalConfig_t dac_ch1 = {wave1, freq100 / 100.0f, Vrms100 / 100.0f, 0.0f};
            DAC_ChannalConfig_t dac_ch2 = {wave2, freq100 / 100.0f, Vrms100 / 100.0f, 0.0f};
            DAC_ConfigChannel(dac_ch1, dac_ch2, Vdda, waveConfig);
        }

        // TM1638的数码管&LED配置
        uint8_t data[16] = {0};

        FloatToSegments((HAL_GetTick() - VrmsSelectTick < VrmsRemainTime ? Vrms100 : freq100) / 100.0f, data);
        // DoubleToSegments(sTime.Hours * 10000 + sTime.Minutes * 100 + sTime.Seconds + (1000 - sTime.SubSeconds * 1000 / (sTime.SecondFraction + 1)) * 0.001, data);
        for(int i = 0; i < 8; i++)
        {
            data[i * 2 + 1] = TM1638_KeyStatus(3, i + 1) == KEY_LONGPRESS ? 0xFF : 0xFE;
        }
        TM1638_DisplayDigits(data);
    }
    TASK_END(WAVECTRL)

    TASK_START(LED, 30)
    {
        HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
        HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

        const uint8_t fractional_precision = 4;
        const float pow10fp = powf(10.0f, fractional_precision);
        int Vdda_t1 = truncf(Vdda), Vdda_t100 = truncf(Vdda * pow10fp) - Vdda_t1 * pow10fp;
        int Vbat_t1 = truncf(Vbat), Vbat_t100 = truncf(Vbat * pow10fp) - Vbat_t1 * pow10fp;
        int Temp_t1 = truncf(Temp), Temp_t100 = truncf(Temp * pow10fp) - Temp_t1 * pow10fp;

        char* dac_ch1_wave;
        switch(wave1)
        {
            case SINE: dac_ch1_wave = "SINE"; break;
            case TRIANGLE: dac_ch1_wave = "TRIANGLE"; break;
            case SQUARE: dac_ch1_wave = "SQUARE"; break;
            case SAWTOOTH: dac_ch1_wave = "SAWTOOTH"; break;
            case DC: dac_ch1_wave = "DC"; break;
        }

        printf("\033[2J\033[1H\n");
        printf("%2d-%2d-%2d %2d:%02d:%02d.%03ld\n", sDate.Year, sDate.Month, sDate.Date, sTime.Hours, sTime.Minutes, sTime.Seconds, 1000 - sTime.SubSeconds * 1000 / (sTime.SecondFraction + 1));
        printf("Vdda: %d.%0*d, Vbat: %d.%0*d, Temp: %d.%0*d\n", Vdda_t1, fractional_precision, Vdda_t100, Vbat_t1, fractional_precision, Vbat_t100, Temp_t1, fractional_precision, Temp_t100);
        printf("dac_ch1: wave: %s, freq: %d.%02d, Vrms: %d.%02d\n", dac_ch1_wave, freq100 / 100, freq100 % 100, Vrms100 / 100, Vrms100 % 100);
        printf("\n");

        // HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);

    }
    TASK_END(LED)
    __WFE();
}

static uint32_t Int100Digits(uint32_t v100)
{
    if (v100 < 1000)       return 1;
    if (v100 < 10000)      return 10;
    if (v100 < 100000)     return 100;
    if (v100 < 1000000)    return 1000;
    if (v100 < 10000000)   return 10000;
    if (v100 < 100000000)  return 100000;
    if (v100 < 1000000000) return 1000000;
    return 1000000;
}
