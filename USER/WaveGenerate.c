/* Includes ------------------------------------------------------------------*/
#include "WaveGenerate.h"

#include <arm_math.h>

#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define WaveGenerate_hdac hdac1
#define WaveGenerate_htim1 htim6
#define WaveGenerate_htim2 htim6

#define MAX_WAVE_HZ     100000
#define MIN_WAVE_HZ     0.1
#define DAC_CHANNELS    2
#define DAC_MAXVAL      4095
#define DAC_MINVAL      0
#define MAX_REFRESH_HZ  1000000
#define TABLE_SIZE2     8
#define TABLE_SIZE      (1 << (TABLE_SIZE2))
#define HALF_BUF        256                     // 半缓冲大小，可根据实时性调整
#define FULL_BUF        ((HALF_BUF) * 2)

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static uint16_t table[TABLE_SIZE]; // 查表
static uint16_t dac_buf[FULL_BUF]; // dac输出表
static uint32_t phase_acc  = 0;
static uint32_t phase_step = 0;

/* Private function prototypes -----------------------------------------------*/
__attribute__((always_inline)) static inline uint16_t dac_value_from_phase(uint32_t phase);
static void init_table(const Wave_t* const wave, float Vdda);
static float Vrms2Vp(const Wave_t* const wave);

/* Exported Constants --------------------------------------------------------*/
extern DAC_HandleTypeDef WaveGenerate_hdac;
extern TIM_HandleTypeDef WaveGenerate_htim1;
extern TIM_HandleTypeDef WaveGenerate_htim2;

/* Exported functions --------------------------------------------------------*/
/**
  * @brief      配置DAC输出波形
  * @param[in]  wave        波形参数
  * @param[in]  Vdda        参考电平
  * @param[in]  waveConfig  波形刷新或初始化
  * @retval     none
  */
void Set_Wave(const Wave_t* const wave, float Vdda, uint8_t waveConfig)
{
    if(wave->freq >= MIN_WAVE_HZ && wave->freq <= MAX_WAVE_HZ)
    {
        init_table(wave, Vdda);
        if(waveConfig)
        {
            HAL_TIM_Base_Stop(&WaveGenerate_htim1);
            phase_step = (uint32_t)(wave->freq * (float)(uint32_t)(-1) / 1000000.0f);
            HAL_DAC_Start_DMA(&WaveGenerate_hdac, DAC_CHANNEL_1, (uint32_t*)dac_buf, FULL_BUF, DAC_ALIGN_12B_R);
            HAL_TIM_Base_Start(&WaveGenerate_htim1);
        }
    }
}

/**
  * @brief      DAC半传输完成回调函数
  * @retval     none
  * @note       更新dac_buf前半区的值
  */
void DAC_ConvHalfCpltCallbackCh1(void)
{
    uint32_t pacc = phase_acc;
    uint32_t pstep = phase_step;
    for(int i = 0; i < HALF_BUF; i++)
    {
        pacc += pstep;
        dac_buf[i] = dac_value_from_phase(pacc);
    }
    phase_acc = pacc;
}

/**
  * @brief      DAC传输完成回调函数
  * @retval     none
  * @note       更新dac_buf后半区的值
  */
void DAC_ConvCpltCallbackCh1(void)
{
    uint32_t pacc = phase_acc;
    uint32_t pstep = phase_step;
    for(int i = HALF_BUF; i < FULL_BUF; i++)
    {
        pacc += pstep;
        dac_buf[i] = dac_value_from_phase(pacc);
    }
    phase_acc = pacc;
}

/**
  * @brief      按phase从table取DAC数值
  * @param[in]  phase   相位，[0, 2^32-1)映射到[0, 2π)
  * @retval     查表得到的DAC数值
  */
__attribute__((always_inline)) static inline uint16_t dac_value_from_phase(uint32_t phase)
{
    return table[phase >> (32 - TABLE_SIZE2)];
}

/**
  * @brief      按波形形状和有效值初始化table，用于取DAC数值
  * @param[in]  wave    波形参数
  * @param[in]  Vdda    参考电平
  * @retval     none
  */
static void init_table(const Wave_t* const wave, float Vdda)
{
    // 生成系数
    const float DAC_MAXVAL_half = DAC_MAXVAL / 2.0f;
    float coef = Vrms2Vp(wave) / (Vdda / 2.0f);
    float Vp = coef * DAC_MAXVAL_half;

    for (int i = 0; i < TABLE_SIZE; i++)
    {
        float rad = 2.0f * PI * i / TABLE_SIZE;
        int16_t value;
        switch(wave->waveForm)
        {
            case SINE:
            {
                value = (arm_sin_f32(rad) * Vp) + DAC_MAXVAL_half;
                break;
            }
            case SQUARE:
            {
                value = (rad < PI ? 1.0f : -1.0f) * Vp + DAC_MAXVAL_half;
                break;
            }
            case TRIANGLE:
            {
                value = 2.0f / PI * (rad < PI / 2.0f ? rad : rad < 1.5f * PI ? PI - rad : rad - 2.0f * PI) * Vp + DAC_MAXVAL_half;
                break;
            }
            case SAWTOOTH:
            {
                value = (rad / PI - 1.0f) * Vp + DAC_MAXVAL_half;
                break;
            }
            case DC:
            default:
            {
                value = DAC_MAXVAL_half;
                break;                    
            }
        }

        if(value > DAC_MAXVAL) value = DAC_MAXVAL;
        if(value < DAC_MINVAL) value = DAC_MINVAL;

        table[i] = (uint16_t)value;
    }
}

/**
  * @brief      转换波形有效值为峰值
  * @param[in]  wave    波形参数
  * @retval     波形峰值
  */
static float Vrms2Vp(const Wave_t* const wave)
{
    float value = wave->Vrms / 4.0f;
    switch(wave->waveForm)
    {
        case SINE:
        {
            value *= (float)M_SQRT2;
            break;
        }
        case SQUARE:
        {
            break;
        }
        case TRIANGLE:
        case SAWTOOTH:
        {
            value *= (float)M_SQRT3;
            break;
        }
        case DC:
        default:
        {
            break;                    
        }
    }
    return value;
}
