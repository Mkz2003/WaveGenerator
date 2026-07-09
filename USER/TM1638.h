#ifndef __TM1638_H
#define __TM1638_H

#include <stdint.h>

extern const uint8_t segCode[17];

void TM1638_DisplayBrightness(uint8_t on, uint8_t brightness);
uint32_t TM1638_ReadKeys(void);
void TM1638_DisplayDigits(uint8_t data[16]);

#endif	// __TM1638_H
