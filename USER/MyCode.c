/* Includes ------------------------------------------------------------------*/
#include "MyCode.h"

#include <ctype.h>
#include <arm_math.h>
#include <stdio.h>
#include <stdlib.h>

#include "main.h"
#include "stm32f0xx_hal_tim.h"
#include "stm32f0xx_ll_adc.h"

/* Private includes ----------------------------------------------------------*/
#include "WaveGenerate.h"
#include "TM1638.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
const uint32_t VrmsRemainTime = 2000;
const float Vout2 = 3.0f;

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
uint32_t freq100 = 1000 * 100;
uint32_t Vrms100 = 100;
WaveForm_t waveForm1 = SINE;

volatile struct {volatile uint16_t Temp, Vref, Vbat;} adcval = {0};
float Vdda = 0.0f, Vbat = 0.0f, Temp = 0.0f;

/* Private function prototypes -----------------------------------------------*/
static uint32_t Int100Digits(uint32_t v100);

/* Exported Constants --------------------------------------------------------*/
extern ADC_HandleTypeDef hadc;
extern DAC_HandleTypeDef hdac1;
extern TIM_HandleTypeDef htim2;

/* Exported functions --------------------------------------------------------*/
/**
  * @brief      main函数的初始化过程
  * @retval     none
  */
void Setup(void)
{
    __HAL_TIM_SET_AUTORELOAD(&htim2, HAL_TIM_ReadCapturedValue(&htim2, TIM_CHANNEL_1) - 1);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    
    Wave_t wave1 = {waveForm1, freq100 / 100.0f, Vrms100 / 100.0f, 0.0f};
    Set_Wave(&wave1, 3.3f, 1);

    HAL_ADCEx_Calibration_Start(&hadc);
    HAL_ADC_Start_DMA(&hadc, (uint32_t*)&adcval, sizeof(adcval) / sizeof(uint16_t));
}

/**
  * @brief      main函数的循环过程
  * @retval     none
  */
void Loop(void)
{

    TASK_INIT()

    TASK_START(ADCSAMPLING, 1)
    {
        float vrefint_cal_vref = (VREFINT_CAL_VREF / 1000.0);
        float temperature_cal1_temp = TEMPSENSOR_CAL1_TEMP;
        float temperature_cal2_temp = TEMPSENSOR_CAL2_TEMP;
        float vrefint_cal_addr = (*VREFINT_CAL_ADDR);
        float tempsensor_cal1_addr = (*TEMPSENSOR_CAL1_ADDR);
        float tempsensor_cal2_addr = (*TEMPSENSOR_CAL2_ADDR);
        Vdda = vrefint_cal_vref * vrefint_cal_addr / adcval.Vref;
        Vbat = adcval.Vbat * Vdda / (float)(1 << 12) * 2.0f;
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
        const uint32_t Vrms100Max = 450, Vrms100Min = 1;

        // 从TM1638读按键
        TM1638_ReadKeys();

        uint8_t waveConfig = 0;
        static uint32_t VrmsSelectTick = 0x7FFFFFFF;

        // 频率+
        if((TM1638_KeyStatus(1, 1) == KEY_CLICK) | (TM1638_KeyStatus(1, 1) == KEY_LONGPRESS))
        {
            freq100 += Int100Digits(freq100);
            if(freq100 > freq100Max) freq100 = freq100Max;
            waveConfig = 1;
            VrmsSelectTick -= VrmsRemainTime;   // 不让显示Vrms以显示Freq
        }

        // 频率-
        if((TM1638_KeyStatus(1, 2) == KEY_CLICK) | (TM1638_KeyStatus(1, 2) == KEY_LONGPRESS))
        {
            freq100 -= Int100Digits(freq100 - Int100Digits(freq100));
            if(freq100 < freq100Min) freq100 = freq100Min;
            waveConfig = 1;
            VrmsSelectTick -= VrmsRemainTime;   // 不让显示Vrms以显示Freq
        }

        // 电压+
        if((TM1638_KeyStatus(1, 3) == KEY_CLICK) | (TM1638_KeyStatus(1, 3) == KEY_LONGPRESS))
        {
            Vrms100 += Int100Digits(Vrms100);
            if(Vrms100 > Vrms100Max) Vrms100 = Vrms100Max;
            waveConfig = 1;
            VrmsSelectTick = HAL_GetTick();
        }

        // 电压-
        if((TM1638_KeyStatus(1, 4) == KEY_CLICK) | (TM1638_KeyStatus(1, 4) == KEY_LONGPRESS))
        {
            Vrms100 -= Int100Digits(Vrms100 - Int100Digits(Vrms100));
            if(Vrms100 < Vrms100Min) Vrms100 = Vrms100Min;
            waveConfig = 1;
            VrmsSelectTick = HAL_GetTick();
        }

        // 波形切换
        static uint8_t waveSelect_entryFlag = 1;    // 令按键单击和长按只触发一次事件
        if((TM1638_KeyStatus(2, 1) == KEY_CLICK) | (TM1638_KeyStatus(2, 1) == KEY_LONGPRESS))
        {
            if(waveSelect_entryFlag != 0)
            {
                WaveForm_t* waveForm = &waveForm1;
                switch(*waveForm)
                {
                    case SINE:
                    {
                        *waveForm = TRIANGLE;
                        break;
                    }
                    case TRIANGLE:
                    {
                        *waveForm = SQUARE;
                        break;
                    }
                    case SQUARE:
                    {
                        *waveForm = SAWTOOTH;
                        break;
                    }
                    case SAWTOOTH:
                    default:
                    {
                        *waveForm = SINE;
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
            Wave_t wave1 = {waveForm1, freq100 / 100.0f, Vrms100 / 100.0f, 0.0f};
            Set_Wave(&wave1, Vdda, waveConfig);
            // uint32_t arr = (uint32_t)((HAL_TIM_ReadCapturedValue(&htim2, TIM_CHANNEL_1) + 1) / Vout2 * Vdda) - 1;
            // if(arr < 10000) __HAL_TIM_SetAutoreload(&htim2, arr);
        }

        // TM1638的数码管&LED配置
        uint16_t data[8] = {0};

        // 显示频率或幅值
        uint8_t flag = HAL_GetTick() - VrmsSelectTick >= VrmsRemainTime;
        TM1638_FloatToSegments((flag ? freq100 : Vrms100) / 100.0f, data);
        TM1638_WriteSegments((flag ? 5 : 6), 7, data);

        // 显示波形
        switch(waveForm1)
        {
            case SAWTOOTH:
            {
                TM1638_WriteSegments(7, 7, data);
                break;
            }
            case SQUARE:
            {
                TM1638_WriteSegments(8, 7, data);
                break;
            }
            case TRIANGLE:
            {
                TM1638_WriteSegments(9, 7, data);
                break;
            }
            case SINE:
            {
                TM1638_WriteSegments(10, 7, data);
                break;
            }
            default:
            {
                break;
            }
        }

        // 输出显示
        TM1638_Display(data);
    }
    TASK_END(WAVECTRL)

    __WFE();
}

/**
  * @brief      对于无符号整数，返回它的数量级
  * @param[in]  v100    无符号整数（放大到100倍）
  * @retval     数量级
  */
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
