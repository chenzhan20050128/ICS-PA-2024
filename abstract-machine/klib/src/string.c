#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)
// change in 0930 14:32
size_t strlen(const char *s)
{
  const char *p = s;
  while (*p)
    p++;        // 遍历到字符串的末尾（\0）
  return p - s; // 返回字符串的长度
}

char *strcpy(char *dst, const char *src)
{
  char *ret = dst;
  while ((*dst++ = *src++))
    ; // 将src的内容逐字节复制到dst，最后自动加上 \0
  return ret;
}

char *strncpy(char *dst, const char *src, size_t n)
{
  char *ret = dst;
  while (n && (*dst++ = *src++))
    n--;
  while (n--)
    *dst++ = '\0'; // 如果src不够n个字符，用\0填充
  return ret;
}

char *strcat(char *dst, const char *src)
{
  char *ret = dst;
  while (*dst)
    dst++; // 移动到dst的末尾
  while ((*dst++ = *src++))
    ; // 追加src的内容
  return ret;
}

int strcmp(const char *s1, const char *s2)
{
  while (*s1 && (*s1 == *s2))
  {
    s1++;
    s2++;
  }
  return *(unsigned char *)s1 - *(unsigned char *)s2;
}

int strncmp(const char *s1, const char *s2, size_t n)
{
  while (n && *s1 && (*s1 == *s2))
  {
    s1++;
    s2++;
    n--;
  }
  if (n == 0)
    return 0;
  return *(unsigned char *)s1 - *(unsigned char *)s2;
}

void *memset(void *s, int c, size_t n)
{

  unsigned char *p = (unsigned char *)s;
  while (n--)
    *p++ = (unsigned char)c; // 取其最低的 8 位,在 C 语言的实际实现中也是如此
  return s;
}
void *memmove(void *dst, const void *src, size_t n)
{
  unsigned char *d = (unsigned char *)dst;
  const unsigned char *s = (const unsigned char *)src;

  // 如果目标地址在源地址的前面，正常从前往后复制
  if (d < s)
  {
    while (n--)
      *d++ = *s++;
  }
  else
  {
    // 如果目标地址在源地址的后面，为了避免重叠，从后往前复制
    d += n;
    s += n;
    while (n--)
      *--d = *--s;
  }

  // 返回目标地址
  return dst;
}

void *memcpy(void *out, const void *in, size_t n)
{
  // memcpy 函数用于复制内存区域的数据，但不处理重叠情况!
  unsigned char *d = (unsigned char *)out;
  const unsigned char *s = (const unsigned char *)in;
  while (n--)
    *d++ = *s++;
  return out;
}

int memcmp(const void *s1, const void *s2, size_t n)
{
  const unsigned char *p1 = (const unsigned char *)s1;
  const unsigned char *p2 = (const unsigned char *)s2;
  while (n--)
  {
    if (*p1 != *p2)
      return *p1 - *p2;
    p1++;
    p2++;
  }
  return 0;
}

#endif
