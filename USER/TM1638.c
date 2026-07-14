#include "TM1638.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "MyCode.h"

// STB控制宏
#define TM1638_STB_LOW()   HAL_GPIO_WritePin(TM1638_STB_GPIO_Port, TM1638_STB_Pin, GPIO_PIN_RESET)
#define TM1638_STB_HIGH()  HAL_GPIO_WritePin(TM1638_STB_GPIO_Port, TM1638_STB_Pin, GPIO_PIN_SET)

#define TM1638_hspi hspi2
extern SPI_HandleTypeDef TM1638_hspi;

const uint8_t segNum = 6;
const uint8_t fractional_precision = 2;
const uint8_t segCode[10] = {
    0x3F, // 0
    0x06, // 1
    0x5B, // 2
    0x4F, // 3
    0x66, // 4
    0x6D, // 5
    0x7D, // 6
    0x07, // 7
    0x7F, // 8
    0x6F  // 9
};

uint8_t TM1638_KeyStatus(uint32_t keyValue, uint8_t k, uint8_t ks)
{
    return (k >= 1 && k <= 3 && ks >= 1 && ks <= 8) ? !!(keyValue & ((0x1 << (ks * 4 - 1)) >> k)) : 0;
}


// 发送1字节
static inline void TM1638_WriteByte(uint8_t data)
{
    HAL_SPI_Transmit(&TM1638_hspi, &data, 1, HAL_MAX_DELAY);
}

// 发送带起始/停止条件的命令
static inline void TM1638_SendCommand(uint8_t cmd)
{
    TM1638_STB_LOW();   // 开始通信
    TM1638_WriteByte(cmd);
    TM1638_STB_HIGH();  // 结束通信
}

// 发送调整显示亮度的命令，范围0~8
void TM1638_DisplayBrightness(uint8_t brightness)
{
    if(brightness > 8) return;
    uint8_t cmd = 0x80 | ((brightness + 7) & 0x0F);
    TM1638_SendCommand(cmd);
}

uint32_t TM1638_ReadKeys(void)
{
    uint32_t keyValue = 0;
    
    TM1638_STB_LOW();          // 拉低STB，启动帧
    
    TM1638_WriteByte(0x42); // 发送读命令
    
    HAL_SPI_Receive(&TM1638_hspi, (uint8_t*)&keyValue, 4, HAL_MAX_DELAY); // 读取按键数据

    TM1638_STB_HIGH();         // 拉高STB，结束帧

    // 多键按下检测，关闭所有灯光
    if((keyValue & (keyValue - 1)) != 0) TM1638_DisplayBrightness(0); else TM1638_DisplayBrightness(8);

    return keyValue;
}

void TM1638_DisplayDigits(uint8_t data[16])
{
    TM1638_SendCommand(0x40); // 自动地址递增命令
    TM1638_STB_LOW();
    TM1638_WriteByte(0xC0);   // 起始地址
    HAL_SPI_Transmit(&TM1638_hspi, data, 16, HAL_MAX_DELAY);
    TM1638_STB_HIGH();
}

void DoubleToSegments(float value, uint8_t data[16])
{
    char s[segNum + 2]; // 预留小数点和'\0'标识
    char* ps = s;
    uint8_t* pdata = data;

    if(segNum > 8) return;

    // snprintf(s, sizeof(s), "%#*.*f", segNum + 1, 2, value);

    char sign[2] = "";
    if(value < 0)
    {
        value = -value;
        sign[0] = '-';    
    }

    float pow10fp = powf(10.0f, fractional_precision);
    int value_t1 = truncf(value);
    int value_t100 = truncf(value * pow10fp) - value_t1 * pow10fp;

    int len = snprintf(NULL, 0, "%s%d.%0*d", sign, value_t1, fractional_precision, value_t100);
    if(len > sizeof(s) - 1) len = sizeof(s) - 1;
    snprintf(s, sizeof(s), "%*s%s%d.%0*d", sizeof(s) - 1 - len, "", sign, value_t1, fractional_precision, value_t100);

    while(*ps != '\0' && pdata < data + segNum * 2)
    {
        if(isdigit((uint8_t)*ps))
        {
            *pdata = segCode[*ps - '0'];
            if(*(ps + 1) == '.')
            {
                *pdata |= 0x80;
                ps += 1;
            }
        }
        else if(*ps == '-')
        {
            *pdata = 0x40;
        }
        pdata += 2;
        ps += 1;
    }
}
