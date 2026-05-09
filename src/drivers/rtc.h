/* src/drivers/rtc.h */
#ifndef RTC_H
#define RTC_H

#include <stdint.h>

void rtc_init(void);
void rtc_update_cache(void);
void rtc_get_time(uint8_t *second, uint8_t *minute, uint8_t *hour, uint8_t *day, uint8_t *month, uint32_t *year);
void rtc_set_time(uint8_t hour, uint8_t minute, uint8_t day, uint8_t month, uint32_t year);

#endif
