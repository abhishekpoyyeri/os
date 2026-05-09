/* src/drivers/rtc.c */
#include "rtc.h"
#include "io.h"

#define CMOS_ADDRESS 0x70
#define CMOS_DATA    0x71

static uint8_t cache_second;
static uint8_t cache_minute;
static uint8_t cache_hour;
static uint8_t cache_day;
static uint8_t cache_month;
static uint32_t cache_year;

static int get_update_in_progress_flag() {
      outb(CMOS_ADDRESS, 0x0A);
      return (inb(CMOS_DATA) & 0x80);
}

static uint8_t get_rtc_register(int reg) {
      outb(CMOS_ADDRESS, reg);
      return inb(CMOS_DATA);
}

static void set_rtc_register(uint8_t reg, uint8_t val) {
    /* Set bit 7 of reg to disable NMI during update */
    outb(CMOS_ADDRESS, reg | 0x80);
    outb(CMOS_DATA, val);
}

static uint8_t bin2bcd(uint8_t val) {
    return ((val / 10) << 4) | (val % 10);
}

void rtc_init(void) {
    rtc_update_cache();
}

void rtc_update_cache(void) {
      uint8_t second, minute, hour, day, month;
      uint32_t year;
      uint8_t last_second, last_minute, last_hour, last_day, last_month, last_year;
      uint8_t registerB;

      while (get_update_in_progress_flag());
      second = get_rtc_register(0x00);
      minute = get_rtc_register(0x02);
      hour = get_rtc_register(0x04);
      day = get_rtc_register(0x07);
      month = get_rtc_register(0x08);
      year = get_rtc_register(0x09);

      do {
            last_second = second;
            last_minute = minute;
            last_hour = hour;
            last_day = day;
            last_month = month;
            last_year = (uint8_t)year;

            while (get_update_in_progress_flag());
            second = get_rtc_register(0x00);
            minute = get_rtc_register(0x02);
            hour = get_rtc_register(0x04);
            day = get_rtc_register(0x07);
            month = get_rtc_register(0x08);
            year = get_rtc_register(0x09);
      } while( (last_second != second) || (last_minute != minute) || (last_hour != hour) ||
               (last_day != day) || (last_month != month) || (last_year != (uint8_t)year) );

      registerB = get_rtc_register(0x0B);

      if (!(registerB & 0x04)) {
            second = (second & 0x0F) + ((second / 16) * 10);
            minute = (minute & 0x0F) + ((minute / 16) * 10);
            hour = ( (hour & 0x0F) + (((hour & 0x70) / 16) * 10) ) | (hour & 0x80);
            day = (day & 0x0F) + ((day / 16) * 10);
            month = (month & 0x0F) + ((month / 16) * 10);
            year = (year & 0x0F) + ((year / 16) * 10);
      }

      if (!(registerB & 0x02) && (hour & 0x80)) {
            hour = ((hour & 0x7F) + 12) % 24;
      }

      year += 2000;

      cache_second = second;
      cache_minute = minute;
      cache_hour = hour;
      cache_day = day;
      cache_month = month;
      cache_year = year;
}

void rtc_get_time(uint8_t *second, uint8_t *minute, uint8_t *hour, uint8_t *day, uint8_t *month, uint32_t *year) {
    if (second) *second = cache_second;
    if (minute) *minute = cache_minute;
    if (hour) *hour = cache_hour;
    if (day) *day = cache_day;
    if (month) *month = cache_month;
    if (year) *year = cache_year;
}

void rtc_set_time(uint8_t hour, uint8_t minute, uint8_t day, uint8_t month, uint32_t year) {
    uint8_t registerB = get_rtc_register(0x0B);
    uint8_t bcd = !(registerB & 0x04);

    uint8_t w_hour = hour, w_minute = minute, w_day = day, w_month = month;
    uint8_t w_year = (year >= 2000) ? (year - 2000) : year;

    if (bcd) {
        w_hour = bin2bcd(hour);
        w_minute = bin2bcd(minute);
        w_day = bin2bcd(day);
        w_month = bin2bcd(month);
        w_year = bin2bcd(w_year);
    }

    asm volatile("cli");

    set_rtc_register(0x04, w_hour);
    set_rtc_register(0x02, w_minute);
    set_rtc_register(0x07, w_day);
    set_rtc_register(0x08, w_month);
    set_rtc_register(0x09, w_year);

    /* Re-enable NMI by reading a register with bit 7 cleared */
    outb(CMOS_ADDRESS, 0x0D);
    inb(CMOS_DATA);
    
    asm volatile("sti");

    rtc_update_cache();
}
