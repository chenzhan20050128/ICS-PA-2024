#ifndef RISCV_H__
#define RISCV_H__

#include <stdint.h>
// 定义一个内联函数 inb，用于读取 8 位（字节）寄存器数据
static inline uint8_t inb(uintptr_t addr) { return *(volatile uint8_t *)addr; }
static inline uint16_t inw(uintptr_t addr) { return *(volatile uint16_t *)addr; }
static inline uint32_t inl(uintptr_t addr) { return *(volatile uint32_t *)addr; }
// 定义一个内联函数 outb，用于写入 8 位（字节）数据到寄存器
static inline void outb(uintptr_t addr, uint8_t data) { *(volatile uint8_t *)addr = data; }
static inline void outw(uintptr_t addr, uint16_t data) { *(volatile uint16_t *)addr = data; }
static inline void outl(uintptr_t addr, uint32_t data) { *(volatile uint32_t *)addr = data; }
// add comments by cz
//  定义若干页表项权限标志常量
#define PTE_V 0x01 // 有效页
#define PTE_R 0x02 // 可读
#define PTE_W 0x04 // 可写
#define PTE_X 0x08 // 可执行
#define PTE_U 0x10 // 用户模式
#define PTE_A 0x40 // 已访问
#define PTE_D 0x80 // 已修改

// 定义权限模式的枚举类型
enum
{
    MODE_U,
    MODE_S,
    MODE_M = 3
}; // 用户模式、超级模式、机器模式

#define MSTATUS_MXR (1 << 19)
#define MSTATUS_SUM (1 << 18)

#if __riscv_xlen == 64
#define MSTATUS_SXL (2ull << 34)
#define MSTATUS_UXL (2ull << 32)
#else
#define MSTATUS_SXL 0
#define MSTATUS_UXL 0
#endif

#endif
