/* Includes ------------------------------------------------------------------*/
#include "WaveGenerate.h"

#include <math.h>

#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
typedef enum
{
    DAC_CH1 = 0,
    DAC_CH2 = 1
} DAC_Channel_t;

/* Private define ------------------------------------------------------------*/
#define WaveGenerate_hdac hdac1
#define WaveGenerate_htim1 htim6
#define WaveGenerate_htim2 htim7

#define TABLE_SIZE      256
#define MAX_WAVE_HZ     100000
#define DAC_MAXVAL      4095
#define DAC_MINVAL      0
#define MAX_REFRESH_HZ  1000000
#define TIM_CLK         64000000

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static int16_t dac_buf[2][2][TABLE_SIZE];

/* Private function prototypes -----------------------------------------------*/
static float Vrms2Vp(float value, Wave_t wave);

/* Exported Constants --------------------------------------------------------*/
extern DAC_HandleTypeDef WaveGenerate_hdac;
extern TIM_HandleTypeDef WaveGenerate_htim1;
extern TIM_HandleTypeDef WaveGenerate_htim2;

/* Exported functions --------------------------------------------------------*/
/**
  * @brief      为DAC通道配置频率、有效值和相位
  * @param[in]  dac_ch1    DAC通道1
  * @param[in]  dac_ch2 DAC通道2
  * @param[in]  Vdda    当前DAC的参考电平（代表DAC_MAXVAL对应的实际电压值）
  * @retval     none
  * @note       使用这个函数配置DAC通道后，DAC配置立刻生效
  */
/**
 * @brief 为DAC通道配置频率、有效值和相位
 * @param ch   通道选择 (DAC_CH1 / DAC_CH2)
 * @param freq 目标频率 (Hz), 0~100000
 * @param phase_deg 初始相位（度）
 */
void DAC_ConfigChannel(DAC_ChannalConfig_t dac_ch1, DAC_ChannalConfig_t dac_ch2, float Vdda)
{

    if(dac_ch1.freq > MAX_WAVE_HZ || dac_ch2.freq > MAX_WAVE_HZ) return;

    static uint8_t bufferCtrl = 1;
    Wave_t wave[2] = {dac_ch1.wave, dac_ch2.wave};
    float freq[2] = {dac_ch1.freq, dac_ch2.freq};
    float phase_deg[2] = {dac_ch1.phase_deg, dac_ch2.phase_deg};
    float Vrms[2] = {dac_ch1.Vrms, dac_ch2.Vrms};

    // 切换缓冲区
    bufferCtrl = 1 - bufferCtrl;

    for(int ch = 0; ch < 2; ch++)
    {
        // 1. 计算点数 N 与刷新率 Fs
        uint32_t N = freq[ch] * TABLE_SIZE <= MAX_REFRESH_HZ ? TABLE_SIZE : MAX_REFRESH_HZ / freq[ch];
        uint32_t Fs = (uint32_t)(freq[ch] * N);

        // 2. 生成带相位偏移的缓冲区
        uint32_t offset = (uint32_t)(phase_deg[ch] / 360.0f * N + 0.5f) % N;  // 四舍五入循环偏移量
        
        // 生成系数
        float coef = Vrms2Vp(Vrms[ch], wave[ch]) / (Vdda / 2.0f);

        for(uint32_t i = 0; i < N; i++)
        {
            float src_index = ((i + offset) % N) * (float)M_TWOPI / N;    // [0, 2π)


            switch(wave[ch])
            {
                case SINE:
                {
                    dac_buf[ch][bufferCtrl][i] = (sinf(src_index) * (coef * DAC_MAXVAL / 2.0f)) + (DAC_MAXVAL / 2.0f);
                    break;
                }
                case SQUARE:
                {
                    dac_buf[ch][bufferCtrl][i] = (src_index < (float)M_PI ? 1.0f : -1.0f) * (coef * DAC_MAXVAL / 2.0f) + (DAC_MAXVAL / 2.0f);
                    break;
                }
                case TRIANGLE:
                {
                    dac_buf[ch][bufferCtrl][i] = (float)M_2_PI * asinf(sinf(src_index)) * (coef * DAC_MAXVAL / 2.0f) + (DAC_MAXVAL / 2.0f);
                    break;
                }
                case SAWTOOTH:
                {
                    dac_buf[ch][bufferCtrl][i] = (src_index / (float)M_PI - 1.0f) * (coef * DAC_MAXVAL / 2.0f) + (DAC_MAXVAL / 2.0f);
                    break;
                }
                case DC:
                default:
                {
                    dac_buf[ch][bufferCtrl][i] = (coef * DAC_MAXVAL / 2.0f) + (DAC_MAXVAL / 2.0f);
                    break;                    
                }
            }

            if(dac_buf[ch][bufferCtrl][i] > DAC_MAXVAL) dac_buf[ch][bufferCtrl][i] = DAC_MAXVAL;
            if(dac_buf[ch][bufferCtrl][i] < DAC_MINVAL) dac_buf[ch][bufferCtrl][i] = DAC_MINVAL;
        }

        // 3. 计算对应定时器的 PSC 和 ARR
        uint32_t prescaler = 0;
        uint32_t period = (TIM_CLK / Fs) - 1;
        while(period > 65535)
        {
            prescaler += 1;
            period = (TIM_CLK / (Fs * (prescaler + 1))) - 1;
        }
        if (period > 65535) period = 65535;

        // 4. 停止对应的定时器和 DAC DMA
        TIM_HandleTypeDef *htim = (ch == DAC_CH1) ? &WaveGenerate_htim1 : &WaveGenerate_htim2;
        uint32_t dac_channel = (ch == DAC_CH1) ? DAC_CHANNEL_1 : DAC_CHANNEL_2;

        // HAL_TIM_Base_Stop(htim);
        HAL_DAC_Stop_DMA(&WaveGenerate_hdac, dac_channel);

        // 5. 更新定时器参数
        __HAL_TIM_SET_PRESCALER(htim, prescaler);
        __HAL_TIM_SET_AUTORELOAD(htim, period);

        // 6. 启动 DMA（循环模式），此时 DAC 已准备好等待触发
        HAL_DAC_Start_DMA(&WaveGenerate_hdac, dac_channel, (uint32_t*)dac_buf[ch][bufferCtrl], N, DAC_ALIGN_12B_R);
    }

    // 确保两个定时器都处于停止状态（DAC_ConfigChannel 已经停止）
    // 同时写入 CEN 位，使 TIM6 和 TIM7 在几乎同一时刻开始计数
    __HAL_TIM_DISABLE(&WaveGenerate_htim1);
    __HAL_TIM_DISABLE(&WaveGenerate_htim2);
    __HAL_TIM_ENABLE(&WaveGenerate_htim1);
    __HAL_TIM_ENABLE(&WaveGenerate_htim2);
}

static float Vrms2Vp(float value, Wave_t wave)
{
    switch(wave)
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
