#include <am.h>
#include <nemu.h>

void __am_timer_init()
{
}

void __am_timer_uptime(AM_TIMER_UPTIME_T *uptime)
{
  // 读取32位的RTC寄存器来获取当前系统时间的微秒数
  uint32_t lo = inl(RTC_ADDR);     // 低32位
  uint32_t hi = inl(RTC_ADDR + 4); // 高32位
  uptime->us = ((uint64_t)hi << 32) | lo;
  
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
