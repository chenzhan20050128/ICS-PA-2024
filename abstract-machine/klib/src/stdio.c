#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)
// the code need to be check! cz 0930 14:45
//  Helper function to convert integer to string
static void itoa(int num, char *str)
{
  char temp[32];
  int i = 0, j = 0, is_negative = 0;

  if (num < 0)
  {
    is_negative = 1;
    num = -num;
  }

  do
  {
    temp[i++] = num % 10 + '0';
    num /= 10;
  } while (num > 0);

  if (is_negative)
  {
    temp[i++] = '-';
  }

  while (i > 0)
  {
    str[j++] = temp[--i];
  }
  str[j] = '\0';
}

int vsprintf(char *out, const char *fmt, va_list ap)
{
  char *p = out;
  const char *f = fmt;

  while (*f)
  {
    if (*f == '%')
    {
      f++;
      if (*f == 's')
      {
        const char *str = va_arg(ap, const char *);
        while (*str)
        {
          *p++ = *str++;
        }
      }
      else if (*f == 'd')
      {
        int num = va_arg(ap, int);
        char num_str[32];
        itoa(num, num_str);
        char *n = num_str;
        while (*n)
        {
          *p++ = *n++;
        }
      }
      else
      {
        *p++ = '%';
        *p++ = *f;
      }
    }
    else
    {
      *p++ = *f;
    }
    f++;
  }

  *p = '\0';
  return p - out;
}

int sprintf(char *out, const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  int ret = vsprintf(out, fmt, ap);
  va_end(ap);
  return ret;
}

int snprintf(char *out, size_t n, const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  int ret = vsnprintf(out, n, fmt, ap);
  va_end(ap);
  return ret;
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap)
{
  char *p = out;
  const char *f = fmt;
  size_t count = 0;

  while (*f && count < n - 1)
  {
    if (*f == '%')
    {
      f++;
      if (*f == 's')
      {
        const char *str = va_arg(ap, const char *);
        while (*str && count < n - 1)
        {
          *p++ = *str++;
          count++;
        }
      }
      else if (*f == 'd')
      {
        int num = va_arg(ap, int);
        char num_str[32];
        itoa(num, num_str);
        char *n = num_str;
        while (*n && count < n - 1)
        {
          *p++ = *n++;
          count++;
        }
      }
      else
      {
        if (count < n - 1)
        {
          *p++ = '%';
          count++;
        }
        if (count < n - 1)
        {
          *p++ = *f;
          count++;
        }
      }
    }
    else
    {
      *p++ = *f;
      count++;
    }
    f++;
  }

  *p = '\0';
  return count;
}

int printf(const char *fmt, ...)
{
  char buffer[1024];
  va_list ap;
  va_start(ap, fmt);
  int ret = vsprintf(buffer, fmt, ap);
  va_end(ap);
  putstr(buffer);
  return ret;
}

#endif