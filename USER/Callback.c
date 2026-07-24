#include "main.h"
#include "WaveGenerate.h"

/* 半传输完成回调：填充前半缓冲 */
void HAL_DAC_ConvHalfCpltCallbackCh1(DAC_HandleTypeDef *hdac)
{
    DAC_ConvHalfCpltCallbackCh1();
}

/* 传输完成回调：填充后半缓冲 */
void HAL_DAC_ConvCpltCallbackCh1(DAC_HandleTypeDef *hdac)
{
    DAC_ConvCpltCallbackCh1();
}
