/* src/drivers/rtc.c */
#include "rtc.h"
#include "io.h"
#include "../kernel/klog.h"

#define CMOS_ADDRESS 0x70
#define CMOS_DATA    0x71
#define CMOS_NMI_DISABLE 0x80

#define RTC_REG_SECOND 0x00
#define RTC_REG_MINUTE 0x02
#define RTC_REG_HOUR   0x04
#define RTC_REG_DAY    0x07
#define RTC_REG_MONTH  0x08
#define RTC_REG_YEAR   0x09
#define RTC_REG_A      0x0A
#define RTC_REG_B      0x0B
#define RTC_REG_D      0x0D

static uint8_t cache_second;
static uint8_t cache_minute;
static uint8_t cache_hour;
static uint8_t cache_day;
static uint8_t cache_month;
static uint32_t cache_year;
static uint8_t rtc_initialized = 0;
static uint8_t last_logged_minute = 0xFF;
static uint32_t rtc_tick_accum = 0;

static uint8_t cmos_read(uint8_t reg, uint8_t disable_nmi) {
    outb(CMOS_ADDRESS, reg | (disable_nmi ? CMOS_NMI_DISABLE : 0));
    return inb(CMOS_DATA);
}

static void cmos_write(uint8_t reg, uint8_t val, uint8_t disable_nmi) {
    outb(CMOS_ADDRESS, reg | (disable_nmi ? CMOS_NMI_DISABLE : 0));
    outb(CMOS_DATA, val);
}

static void cmos_enable_nmi(void) {
    outb(CMOS_ADDRESS, RTC_REG_D);
    inb(CMOS_DATA);
}

static uint8_t rtc_update_in_progress(void) {
    return cmos_read(RTC_REG_A, 0) & 0x80;
}

static uint8_t bcd2bin(uint8_t val) {
    return (uint8_t)((val & 0x0F) + ((val >> 4) * 10));
}

static uint8_t bin2bcd(uint8_t val) {
    return (uint8_t)(((val / 10) << 4) | (val % 10));
}

static void log_raw_cmos(uint8_t second, uint8_t minute, uint8_t hour,
                         uint8_t day, uint8_t month, uint8_t year,
                         uint8_t register_b) {
    klog_info_hex("RTC raw second", second);
    klog_info_hex("RTC raw minute", minute);
    klog_info_hex("RTC raw hour", hour);
    klog_info_hex("RTC raw day", day);
    klog_info_hex("RTC raw month", month);
    klog_info_hex("RTC raw year", year);
    klog_info_hex("RTC register B", register_b);
}

void rtc_init(void) {
    rtc_initialized = 1;
    rtc_update_cache();
}

void rtc_update_cache(void) {
    uint8_t second, minute, hour, day, month, year;
    uint8_t last_second, last_minute, last_hour, last_day, last_month, last_year;
    uint8_t register_b;

    while (rtc_update_in_progress());
    second = cmos_read(RTC_REG_SECOND, 0);
    minute = cmos_read(RTC_REG_MINUTE, 0);
    hour = cmos_read(RTC_REG_HOUR, 0);
    day = cmos_read(RTC_REG_DAY, 0);
    month = cmos_read(RTC_REG_MONTH, 0);
    year = cmos_read(RTC_REG_YEAR, 0);

    do {
        last_second = second;
        last_minute = minute;
        last_hour = hour;
        last_day = day;
        last_month = month;
        last_year = year;

        while (rtc_update_in_progress());
        second = cmos_read(RTC_REG_SECOND, 0);
        minute = cmos_read(RTC_REG_MINUTE, 0);
        hour = cmos_read(RTC_REG_HOUR, 0);
        day = cmos_read(RTC_REG_DAY, 0);
        month = cmos_read(RTC_REG_MONTH, 0);
        year = cmos_read(RTC_REG_YEAR, 0);
    } while (last_second != second || last_minute != minute || last_hour != hour ||
             last_day != day || last_month != month || last_year != year);

    register_b = cmos_read(RTC_REG_B, 0);

    if (!rtc_initialized || minute != last_logged_minute) {
        log_raw_cmos(second, minute, hour, day, month, year, register_b);
        last_logged_minute = minute;
    }

    uint8_t pm = hour & 0x80;
    hour &= 0x7F;

    if (!(register_b & 0x04)) {
        second = bcd2bin(second);
        minute = bcd2bin(minute);
        hour = bcd2bin(hour);
        day = bcd2bin(day);
        month = bcd2bin(month);
        year = bcd2bin(year);
    }

    if (!(register_b & 0x02)) {
        if (pm) {
            if (hour != 12) hour = (uint8_t)(hour + 12);
        } else if (hour == 12) {
            hour = 0;
        }
    }

    cache_second = second;
    cache_minute = minute;
    cache_hour = hour;
    cache_day = day;
    cache_month = month;
    cache_year = 2000 + year;
}

void rtc_tick(void) {
    if (!rtc_initialized) return;
    rtc_tick_accum++;
    if (rtc_tick_accum >= 100) {
        rtc_tick_accum = 0;
        rtc_update_cache();
    }
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
    if (hour > 23 || minute > 59 || day == 0 || day > 31 || month == 0 || month > 12) {
        klog_error("RTC set rejected invalid time/date");
        return;
    }

    uint8_t register_b = cmos_read(RTC_REG_B, 1);
    uint8_t binary_mode = register_b & 0x04;
    uint8_t twenty_four_hour = register_b & 0x02;
    uint8_t year_two_digit = (uint8_t)((year >= 2000) ? (year - 2000) : year);
    uint8_t write_hour = hour;
    uint8_t pm = 0;

    klog_info("RTC set begin: disable NMI and inhibit updates");
    cmos_write(RTC_REG_B, register_b | 0x80, 1);

    if (!twenty_four_hour) {
        if (hour == 0) {
            write_hour = 12;
        } else if (hour == 12) {
            write_hour = 12;
            pm = 0x80;
        } else if (hour > 12) {
            write_hour = (uint8_t)(hour - 12);
            pm = 0x80;
        }
    }

    if (!binary_mode) {
        write_hour = bin2bcd(write_hour);
        minute = bin2bcd(minute);
        day = bin2bcd(day);
        month = bin2bcd(month);
        year_two_digit = bin2bcd(year_two_digit);
    }
    write_hour |= pm;

    cmos_write(RTC_REG_HOUR, write_hour, 1);
    cmos_write(RTC_REG_MINUTE, minute, 1);
    cmos_write(RTC_REG_DAY, day, 1);
    cmos_write(RTC_REG_MONTH, month, 1);
    cmos_write(RTC_REG_YEAR, year_two_digit, 1);

    cmos_write(RTC_REG_B, register_b, 1);
    cmos_enable_nmi();
    klog_info("RTC set commit: updates restored and NMI enabled");

    rtc_update_cache();
}
