#include "WaveGenerate.h"

#include "dac.h"
#include "tim.h"

#include <math.h>

#include "MyCode.h"

#define WaveGenerate_hdac hdac1
#define WaveGenerate_htim1 htim6
#define WaveGenerate_htim2 htim7

extern DAC_HandleTypeDef WaveGenerate_hdac;
extern TIM_HandleTypeDef WaveGenerate_htim1;
extern TIM_HandleTypeDef WaveGenerate_htim2;

#define TABLE_SIZE 1024U
#define MAX_WAVE_HZ 100000U
#define DAC_MAX 4095U
#define MAX_REFRESH_HZ 1000000U
#define TIM_CLK 64000000U

typedef enum
{
    DAC_CH1 = 0,
    DAC_CH2 = 1
} DAC_Channel_t;

static uint16_t dac_buf[2][TABLE_SIZE];   // ch0 对应通道1，ch1 对应通道2

/**
 * @brief 为指定 DAC 通道配置频率和相位（不启动定时器）
 * @param ch   通道选择 (DAC_CH1 / DAC_CH2)
 * @param freq 目标频率 (Hz), 1~100000
 * @param phase_deg 初始相位（度）
 */
void DAC_ConfigChannel(Wave_t wave1, double freq1, double phase_deg1, Wave_t wave2, double freq2, double phase_deg2)
{

    if(freq1 > MAX_WAVE_HZ || freq2 > MAX_WAVE_HZ) return;

    HAL_TIM_Base_Stop(&WaveGenerate_htim1);
    HAL_TIM_Base_Stop(&WaveGenerate_htim2);
    HAL_DAC_Stop_DMA(&WaveGenerate_hdac, DAC_CHANNEL_1);
    HAL_DAC_Stop_DMA(&WaveGenerate_hdac, DAC_CHANNEL_2);

    Wave_t wave[2] = {wave1, wave2};
    double freq[2] = {freq1, freq2};
    double phase_deg[2] = {phase_deg1, phase_deg2};

    for(int ch = 0; ch < 2; ch++)
    {
        // 1. 计算点数 N 与刷新率 Fs
        uint32_t N = freq[ch] * TABLE_SIZE <= MAX_REFRESH_HZ ? TABLE_SIZE : MAX_REFRESH_HZ / freq[ch];
        uint32_t Fs = (uint32_t)(freq[ch] * N);

        // 2. 生成带相位偏移的缓冲区
        uint32_t offset = (uint32_t)(phase_deg[ch] / 360.0 * N + 0.5) % N;  // 四舍五入循环偏移量
        for(uint32_t i = 0; i < N; i++)
        {
            double src_index = ((i + offset) % N) * M_TWOPI / N;    // [0, 2π)

            switch(wave[ch])
            {
                case SINE:
                {
                    dac_buf[ch][i] = (uint16_t)((sin(src_index) * (DAC_MAX / 2.0)) + (DAC_MAX / 2.0));
                    break;
                }
                case SQUARE:
                {
                    dac_buf[ch][i] = (uint16_t)((src_index < M_PI) * DAC_MAX);
                    break;
                }
                case TRIANGLE:
                {
                    dac_buf[ch][i] = (uint16_t)((1.0 - fabs(src_index / M_PI - 1.0)) * DAC_MAX);
                    break;
                }
                case SAWTOOTH:
                {
                    dac_buf[ch][i] = (uint16_t)(src_index / M_TWOPI * DAC_MAX);
                    break;
                }
            }

            if(dac_buf[ch][i] >= DAC_MAX) dac_buf[ch][i] = DAC_MAX;
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

        HAL_TIM_Base_Stop(htim);
        HAL_DAC_Stop_DMA(&WaveGenerate_hdac, dac_channel);

        // 5. 更新定时器参数
        __HAL_TIM_SET_PRESCALER(htim, prescaler);
        __HAL_TIM_SET_AUTORELOAD(htim, period);

        // 6. 启动 DMA（循环模式），此时 DAC 已准备好等待触发
        HAL_DAC_Start_DMA(&WaveGenerate_hdac, dac_channel, (uint32_t*)dac_buf[ch], N, DAC_ALIGN_12B_R);
    }

    // 确保两个定时器都处于停止状态（DAC_ConfigChannel 已经停止）
    // 同时写入 CEN 位，使 TIM6 和 TIM7 在几乎同一时刻开始计数
    __HAL_TIM_ENABLE(&WaveGenerate_htim1);
    __HAL_TIM_ENABLE(&WaveGenerate_htim2);
}
