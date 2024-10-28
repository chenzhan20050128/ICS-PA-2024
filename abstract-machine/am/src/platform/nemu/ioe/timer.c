#include <am.h>
#include <nemu.h>
void __am_timer_init()
{
}

void __am_timer_uptime(AM_TIMER_UPTIME_T *uptime)
{
  // printf("%d \n", RTC_ADDR);
  uint64_t temp = ((uint64_t)inl(RTC_ADDR + 4) << 32) | (uint64_t)inl(RTC_ADDR);
  //printf("%d %d\n", RTC_ADDR, temp);
  uptime->us = temp;
}

void __am_timer_rtc(AM_TIMER_RTC_T *rtc)
{
  rtc->second = 0;
  rtc->minute = 0;
  rtc->hour = 0;
  rtc->day = 0;
  rtc->month = 0;
  rtc->year = 1900;
}
