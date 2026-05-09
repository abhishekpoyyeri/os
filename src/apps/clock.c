/* src/apps/clock.c */
#include "../drivers/io.h"
#include <stdint.h>

#define CMOS_ADDRESS 0x70
#define CMOS_DATA    0x71

int get_update_in_progress_flag() {
      outb(CMOS_ADDRESS, 0x0A);
      return (inb(CMOS_DATA) & 0x80);
}

uint8_t get_rtc_register(int reg) {
      outb(CMOS_ADDRESS, reg);
      return inb(CMOS_DATA);
}

void read_rtc(uint8_t *second, uint8_t *minute, uint8_t *hour, uint8_t *day, uint8_t *month, uint32_t *year) {
      uint8_t last_second;
      uint8_t last_minute;
      uint8_t last_hour;
      uint8_t last_day;
      uint8_t last_month;
      uint8_t last_year;
      uint8_t registerB;

      while (get_update_in_progress_flag());
      *second = get_rtc_register(0x00);
      *minute = get_rtc_register(0x02);
      *hour = get_rtc_register(0x04);
      *day = get_rtc_register(0x07);
      *month = get_rtc_register(0x08);
      *year = get_rtc_register(0x09);

      do {
            last_second = *second;
            last_minute = *minute;
            last_hour = *hour;
            last_day = *day;
            last_month = *month;
            last_year = (uint8_t)*year;

            while (get_update_in_progress_flag());
            *second = get_rtc_register(0x00);
            *minute = get_rtc_register(0x02);
            *hour = get_rtc_register(0x04);
            *day = get_rtc_register(0x07);
            *month = get_rtc_register(0x08);
            *year = get_rtc_register(0x09);
      } while( (last_second != *second) || (last_minute != *minute) || (last_hour != *hour) ||
               (last_day != *day) || (last_month != *month) || (last_year != (uint8_t)*year) );

      registerB = get_rtc_register(0x0B);

      // Convert BCD to binary if necessary
      if (!(registerB & 0x04)) {
            *second = (*second & 0x0F) + ((*second / 16) * 10);
            *minute = (*minute & 0x0F) + ((*minute / 16) * 10);
            *hour = ( (*hour & 0x0F) + (((*hour & 0x70) / 16) * 10) ) | (*hour & 0x80);
            *day = (*day & 0x0F) + ((*day / 16) * 10);
            *month = (*month & 0x0F) + ((*month / 16) * 10);
            *year = (*year & 0x0F) + ((*year / 16) * 10);
      }

      // Convert 12 hour clock to 24 hour clock if necessary
      if (!(registerB & 0x02) && (*hour & 0x80)) {
            *hour = ((*hour & 0x7F) + 12) % 24;
      }

      // Calculate the full (4-digit) year
      *year += 2000;
}
