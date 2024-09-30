#ifndef ARCH_H__
#define ARCH_H__

#ifdef __riscv_e
#define NR_REGS 16
#else
#define NR_REGS 32
#endif
//change in 0930 14:02
//corresponding to /home/chenzhan/Desktop/ics2024/abstract-machine/am/src/riscv/nemu/trap.S
struct Context {
  // Order these to match the offsets used in trap.S
  uintptr_t gpr[NR_REGS]; // General purpose registers
  uintptr_t mcause;       // Offset corresponds to OFFSET_CAUSE
  uintptr_t mstatus;      // Offset corresponds to OFFSET_STATUS
  uintptr_t mepc;         // Offset corresponds to OFFSET_EPC
  void *pdir;             // Page directory (if used)
};


#ifdef __riscv_e
#define GPR1 gpr[15] // a5
#else
#define GPR1 gpr[17] // a7
#endif

#define GPR2 gpr[0]
#define GPR3 gpr[0]
#define GPR4 gpr[0]
#define GPRx gpr[0]

#endif
