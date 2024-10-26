#include <am.h>
#include <klib.h>
#include <klib-macros.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)
static unsigned long int next = 1;

// 保存当前分配内存的起始位置
static uintptr_t addr = 0;

// 初始化堆起始地址 add by cz at 1026 20:31
void init_heap()
{
  addr = (uintptr_t)heap.start;
}

// 实现简单的 malloc() 函数
void *malloc(size_t size)
{
  // 确保 malloc 不在初始化期间被调用，避免递归错误
#if !(defined(__ISA_NATIVE__) && defined(__NATIVE_USE_KLIB__))
  if (addr == 0)
  {
    // panic("Heap not initialized");
  }

  // 将请求的内存大小对齐到 8 字节（或其他对齐要求）
  size = ROUNDUP(size, 8);

  // 检查是否有足够的空间
  if (addr + size > (uintptr_t)heap.end)
  {
    panic("Out of memory");
    return NULL;
  }

  // 保存当前地址作为返回值，并更新 addr
  void *allocated = (void *)addr;
  addr += size;

  return allocated;
#else
  return NULL;
#endif
}

int rand(void)
{
  // RAND_MAX assumed to be 32767
  next = next * 1103515245 + 12345;
  return (unsigned int)(next / 65536) % 32768;
}

void srand(unsigned int seed)
{
  next = seed;
}

int abs(int x)
{
  return (x < 0 ? -x : x);
}

int atoi(const char *nptr)
{
  int x = 0;
  while (*nptr == ' ')
  {
    nptr++;
  }
  while (*nptr >= '0' && *nptr <= '9')
  {
    x = x * 10 + *nptr - '0';
    nptr++;
  }
  return x;
}

void free(void *ptr)
{
}

#endif
