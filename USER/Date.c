#include "Date.h"

#include <stdio.h>
#include <string.h>
#include <main.h>

extern RTC_HandleTypeDef hrtc;

void RTCTimeInit(void)
{	
	uint8_t YEAR = ((((__DATE__ [7] - '0') * 10 + (__DATE__ [8] - '0')) * 10 + (__DATE__ [9] - '0')) * 10 + (__DATE__ [10] - '0')) - 2000;
	uint8_t MONTH = ( __DATE__ [2] == 'n' ? (__DATE__ [1] == 'a' ? 1 : 6) \
	: __DATE__ [2] == 'b' ? 2 \
	: __DATE__ [2] == 'r' ? (__DATE__ [0] == 'M' ? 3 : 4) \
	: __DATE__ [2] == 'y' ? 5 \
	: __DATE__ [2] == 'n' ? 6 \
	: __DATE__ [2] == 'l' ? 7 \
	: __DATE__ [2] == 'g' ? 8 \
	: __DATE__ [2] == 'p' ? 9 \
	: __DATE__ [2] == 't' ? 10 \
	: __DATE__ [2] == 'v' ? 11 : 12);
	uint8_t DATE = ((__DATE__ [4] == ' ' ? 0 : ((__DATE__ [4] - '0') * 10 )) + (__DATE__ [5] - '0'));
	uint8_t HOURS = ((__TIME__ [0] == ' ' ? 0 : ((__TIME__ [0] - '0') * 10 )) + (__TIME__ [1] - '0'));
	uint8_t MINUTES = ((__TIME__ [3] == ' ' ? 0 : ((__TIME__ [3] - '0') * 10 )) + (__TIME__ [4] - '0'));
	uint8_t SECONDS = ((__TIME__ [6] == ' ' ? 0 : ((__TIME__ [6] - '0') * 10 )) + (__TIME__ [7] - '0'));

	RTC_DateTypeDef sDate = {
		.Date = DATE,
		.Month = MONTH,
		.Year = YEAR
	};
	RTC_TimeTypeDef sTime = {
		.Hours = HOURS,
		.Minutes = MINUTES,
		.Seconds = SECONDS
	};
	HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
	HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
}
