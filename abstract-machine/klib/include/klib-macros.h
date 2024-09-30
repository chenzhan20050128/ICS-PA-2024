#ifndef KLIB_MACROS_H__
#define KLIB_MACROS_H__
// add comments by cz 0930 14:19
//  定义宏：用于将一个地址向上取整到 sz 的倍数
//  例如：当地址 a 不是 sz 的整数倍时，增加到下一倍数
#define ROUNDUP(a, sz) ((((uintptr_t)a) + (sz) - 1) & ~((sz) - 1))

// 定义宏：用于将一个地址向下取整到 sz 的倍数
// 例如：当地址 a 不是 sz 的整数倍时，减少到前一倍数
#define ROUNDDOWN(a, sz) ((((uintptr_t)a)) & ~((sz) - 1))

// 定义宏：计算数组的元素个数
// 使用 sizeof 运算符，数组总字节数除以单个元素字节数
#define LENGTH(arr) (sizeof(arr) / sizeof((arr)[0]))

// 定义宏：创建一个表示范围的结构体 Area
// 起始地址为 st，结束地址为 ed
#define RANGE(st, ed) \
  (Area) { .start = (void *)(st), .end = (void *)(ed) }

// 定义宏：判断指针 ptr 是否在给定的 Area 范围内
// 当 ptr 地址在 start 和 end 之间返回 true
#define IN_RANGE(ptr, area) ((area).start <= (ptr) && (ptr) < (area).end)

// 定义宏：将符号 s 转换成字符串
// 使用 # 运算符将符号字面化
#define STRINGIFY(s) #s

// 定义宏：间接调用 STRINGIFY 宏，实现两次字符串化（用于宏展开）
#define TOSTRING(s) STRINGIFY(s)

// 定义宏：连接两个符号 x 和 y
// 使用 ## 运算符实现符号的拼接
#define _CONCAT(x, y) x##y

// 定义宏：间接调用 _CONCAT 宏，实现符号连接
#define CONCAT(x, y) _CONCAT(x, y)

// 定义宏：打印字符串 s
// 遍历字符串 s 的每个字符，调用 putch 函数输出字符
#define putstr(s) \
  ({ for (const char *p = s; *p; p++) putch(*p); })

// 定义宏：从 IO 设备读取值到变量
// 使用指定寄存器的类型定义变量，调用 ioe_read 函数读取值
#define io_read(reg) \
  ({ reg##_T __io_param; \
    ioe_read(reg, &__io_param); \
    __io_param; })

// 定义宏：向 IO 设备写入值
// 初始化寄存器类型的变量，调用 ioe_write 函数写入值
#define io_write(reg, ...) \
  ({ reg##_T __io_param = (reg##_T) { __VA_ARGS__ }; \
    ioe_write(reg, &__io_param); })

// 定义宏：静态断言验证条件 const_cond
// 如果 const_cond 为假，则数组大小为负，导致编译错误
#define static_assert(const_cond) \
  static char CONCAT(_static_assert_, __LINE__)[(const_cond) ? 1 : -1] __attribute__((unused))

// 定义宏：检测条件 cond 是否为真，若为真则输出错误信息 s 并调用 halt
// 输出包含文件名和行号的信息，然后终止程序
#define panic_on(cond, s) \
  ({ if (cond) { \
      putstr("AM Panic: "); putstr(s); \
      putstr(" @ " __FILE__ ":" TOSTRING(__LINE__) "  \n"); \
      halt(1); \
    } })

// 定义宏：直接调用 panic_on 宏，始终触发恐慌
// 传递条件恒为真的 panic_on 简化调用
#define panic(s) panic_on(1, s)

#endif