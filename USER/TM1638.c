/* Includes ------------------------------------------------------------------*/
#include "TM1638.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/**
  * @brief  按键状态存储结构体
  */
typedef struct
{
    uint8_t pressCount;
    uint8_t isRead;
    KeyStatus_t keyStatus;
} Key_t;    // (k, ks)

/* Private define ------------------------------------------------------------*/
#define TM1638_hspi hspi2
static const uint8_t longpressCount = 10;  // 判断单击与长按的阈值(ms)
static const uint8_t segNum = 8;   // 数码管段数
static const uint8_t fractional_precision = 4; // 向数码管输出浮点数的小数位数

/* Private macro -------------------------------------------------------------*/
#define TM1638_STB_LOW()   HAL_GPIO_WritePin(TM1638_STB_GPIO_Port, TM1638_STB_Pin, GPIO_PIN_RESET)
#define TM1638_STB_HIGH()  HAL_GPIO_WritePin(TM1638_STB_GPIO_Port, TM1638_STB_Pin, GPIO_PIN_SET)

/* Private variables ---------------------------------------------------------*/
static const uint8_t segCode[10] = // 数字到数码管显示数据的映射
{
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

static Key_t key[4][8] = {0};

/* Private function prototypes -----------------------------------------------*/
static void TM1638_WriteByte(uint8_t data);
static void TM1638_SendCommand(uint8_t cmd);

/* Exported Constants --------------------------------------------------------*/
extern SPI_HandleTypeDef TM1638_hspi;

/* Exported functions --------------------------------------------------------*/
/**
  * @brief      发送调整显示亮度的命令
  * @param[in]  brightness    显示亮度，范围0~8
  * @retval     none
  */
void TM1638_DisplayBrightness(uint8_t brightness)
{
    if(brightness > 8) return;
    uint8_t cmd = 0x80 | ((brightness + 7) & 0x0F);
    TM1638_SendCommand(cmd);
}

/**
  * @brief      从TM1638读取一次按键数据，并更新按键的状态判定
  * @retval     按键读取值
  */
uint32_t TM1638_ReadKeys(void)
{
    uint32_t rxData = 0;
    
    TM1638_STB_LOW();          // 拉低STB，启动帧
    TM1638_WriteByte(0x42); // 发送读命令
    HAL_SPI_Receive(&TM1638_hspi, (uint8_t*)&rxData, 4, 1); // 读取按键数据，若失败则rxData默认0
    TM1638_STB_HIGH();         // 拉高STB，结束帧

    // 多键按下检测，关闭所有灯光
    if((rxData & (rxData - 1)) != 0) TM1638_DisplayBrightness(0); else TM1638_DisplayBrightness(8);

    // 按键状态判定
    for(uint8_t k = 1; k <= 4; k++)
    {
        for(uint8_t ks = 1; ks <= 8; ks++)
        {
            Key_t* pkey = &key[k - 1][ks - 1];

            // 如果外部读取了按键，就清除它的状态
            if(pkey->isRead != 0 && pkey->keyStatus != KEY_RELEASE)
            {
                pkey->keyStatus = KEY_NONE;
                pkey->isRead = 0;
            }

            uint8_t keyValue = !!(rxData & ((0x1 << (ks * 4 - 1)) >> k));   // 第(k, ks)个按键的读取值
            uint8_t pressCount = pkey->pressCount;  // 判断单击与长按的计数变量
            KeyStatus_t keyStatus = pkey->keyStatus;    // 按键现态

            /** 状态机过程 
              *                      ---------------------------------------------------------------
              *                     /          (if LongPress and the key isn't released)            \ 
              *                     |                                                               |
              *                     v                                                               |
              *               -> LongPress -                                                        |
              *    (initial) /              \     (external task(s) read the key status)           /
              *     Release -                ----------------------------------------------> None -
              *        ^     \              /                                                      \        
              *        |      ---> Click ---                                                        |
              *         \                       (if Click or the key is released)                  /
              *          --------------------------------------------------------------------------
              */
            switch(keyStatus)
            {
                case KEY_RELEASE:
                {
                    // 判断 单击 与 长按
                    if(keyValue == 1)
                    {
                        pressCount += 1; 
                    }
                    else if(pressCount > 0 && pressCount < longpressCount)
                    {
                        keyStatus = KEY_CLICK;
                    }

                    // 判断 长按
                    if(pressCount >= longpressCount) keyStatus = KEY_LONGPRESS;
                    break;
                }
                case KEY_NONE:
                {
                    // 判断 维持长按 与 恢复弹起
                    if(keyValue == 1 && pressCount >= longpressCount)
                    {
                        keyStatus = KEY_LONGPRESS;
                    }
                    else 
                    {
                        keyStatus = KEY_RELEASE;
                        pressCount = 0;
                    }
                    break;
                }
                default:
                {
                    break;
                }
            }

            // 更新按键次态
            pkey->pressCount = pressCount;
            pkey->keyStatus = keyStatus;
        }
    }
    return rxData;
}

/**
  * @brief      读取按键状态
  * @param[in]  k    按键的第k列
  * @param[in]  ks   按键的第ks行
  * @retval     按键状态
  * @note       使用这个函数读取按键后，该按键将被标记为已使用，只有使用过的按键才能在TM1638_ReadKeys()内更新状态
  */
KeyStatus_t TM1638_KeyStatus(uint8_t k, uint8_t ks)
{
    KeyStatus_t keyStatus = KEY_NONE;
    if(k >= 1 && k <= 3 && ks >= 1 && ks <= 8)
    {
        Key_t* pkey = &key[k - 1][ks - 1];
        keyStatus = pkey->keyStatus;
        pkey->isRead = 1;
    }
    return keyStatus;
}

/**
  * @brief      发送TM1638传输数组
  * @param[in]  data[16]    TM1638传输数组
  * @retval     none
  */
void TM1638_DisplayDigits(uint8_t data[16])
{
    TM1638_SendCommand(0x40); // 自动地址递增命令
    TM1638_STB_LOW();
    TM1638_WriteByte(0xC0);   // 起始地址
    HAL_SPI_Transmit(&TM1638_hspi, data, 16, HAL_MAX_DELAY);
    TM1638_STB_HIGH();
}

/**
  * @brief      将浮点数化为TM1638格式
  * @param[in]  value       浮点数
  * @param[out] data[16]    TM1638传输数组
  * @retval     none
  */
void FloatToSegments(float value, uint8_t data[16])
{
    char s[segNum + 2]; // 预留小数点和'\0'标识
    char* ps = s;
    uint8_t* pdata = data;

    if(segNum > 8) return;

    // snprintf(s, sizeof(s), "%#*.*f", segNum + 1, fractional_precision, value);
    {
        char sign[2] = "";
        if(value < 0)
        {
            value = -value;
            sign[0] = '-';    
        }

        float pow10fp = powf(10.0f, fractional_precision);
        int value_t1 = truncf(value);
        int value_t100 = truncf(value * pow10fp) - value_t1 * pow10fp;

        // 计算输出字符串长度
        int len = snprintf(NULL, 0, "%s%d.%0*d", sign, value_t1, fractional_precision, value_t100);
        if(len > sizeof(s) - 1) len = sizeof(s) - 1;

        // 转换浮点数为字符串
        snprintf(s, sizeof(s), "%*s%s%d.%0*d", sizeof(s) - 1 - len, "", sign, value_t1, fractional_precision, value_t100);
    }

    // 转换字符串为TM1638格式
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

/**
  * @brief      向TM1638发送1字节
  * @param[in]  data    1字节数据
  * @retval     none
  */
static void TM1638_WriteByte(uint8_t data)
{
    HAL_SPI_Transmit(&TM1638_hspi, &data, 1, HAL_MAX_DELAY);
}

/**
  * @brief      向TM1638发送带起始/停止条件的命令
  * @param[in]  cmd     1字节命令
  * @retval     none
  */
static void TM1638_SendCommand(uint8_t cmd)
{
    TM1638_STB_LOW();
    TM1638_WriteByte(cmd);
    TM1638_STB_HIGH();
}
