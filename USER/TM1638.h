#ifndef __TM1638_H
#define __TM1638_H

#include <stdint.h>

extern const uint8_t segCode[10];

uint8_t TM1638_KeyStatus(uint32_t keyValue, uint8_t k, uint8_t ks);
void TM1638_DisplayBrightness(uint8_t brightness);
uint32_t TM1638_ReadKeys(void);
void TM1638_DisplayDigits(uint8_t data[16]);
void DoubleToSegments(float value, uint8_t data[16]);

#endif	// __TM1638_H
