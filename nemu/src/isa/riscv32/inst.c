#include "local-include/reg.h"
#include <cpu/cpu.h>
#include <cpu/ifetch.h>
#include <cpu/decode.h>

#define R(i) gpr(i)
#define Mr vaddr_read
#define Mw vaddr_write

#define XLEN 32 // 0929 12:36 cz

// Helper function to compute high 32 bits of signed multiplication
static inline int32_t mulh_helper(int32_t a, int32_t b)
{
  int64_t result = (int64_t)a * (int64_t)b;
  return (int32_t)(result >> 32);
}

// Helper function to compute high 32 bits of signed * unsigned multiplication
static inline int32_t mulhsu_helper(int32_t a, uint32_t b)
{
  int64_t result = (int64_t)a * (int64_t)b;
  return (int32_t)(result >> 32);
}

enum
{
  TYPE_I,
  TYPE_U,
  TYPE_S,
  TYPE_R,
  TYPE_B,
  TYPE_J,
  TYPE_N, // none
};

#define src1R()     \
  do                \
  {                 \
    *src1 = R(rs1); \
  } while (0)
#define src2R()     \
  do                \
  {                 \
    *src2 = R(rs2); \
  } while (0)
/*
BITS(x, hi, lo):
目的：提取x中位于hi到lo之间的位，类似于Verilog中的x[hi:lo]。
SEXT(x, len):
目的：执行符号扩展，将一个小于len位的有符号数扩展成len位。
*/
#define immI()                        \
  do                                  \
  {                                   \
    *imm = SEXT(BITS(i, 31, 20), 12); \
  } while (0)
#define immU()                              \
  do                                        \
  {                                         \
    *imm = SEXT(BITS(i, 31, 12), 20) << 12; \
  } while (0)
#define immS()                                               \
  do                                                         \
  {                                                          \
    *imm = (SEXT(BITS(i, 31, 25), 7) << 5) | BITS(i, 11, 7); \
  } while (0)
/*
形成的立即数在描述中为13位（包括符号位），具体的位组合如下：

imm[12]（符号位）: inst[31]
imm[10:5]: inst[30:25]
imm[4:1]: inst[11:8]
imm[11]: inst[7]
组合后立即数形式为 (imm[12], imm[11], imm[10:5], imm[4:1], 0)，即将位置对齐后作为SB格式的13位立即数（注意最低位自动补0，因为分支偏移是字对齐）。
*/
#define immB()                               \
  do                                         \
  {                                          \
    *imm = SEXT((BITS(i, 31, 31) << 12) |    \
                    (BITS(i, 30, 25) << 5) | \
                    (BITS(i, 11, 8) << 1) |  \
                    (BITS(i, 7, 7) << 11),   \
                13);                         \
  } while (0)
/*
在UJ格式中，立即数的编码如下：

符号位：inst[31]
高位部分：inst[30:21]
中间位：inst[20]
低位部分：inst[19:12]
UJ格式中生成的立即数在移动后为21位（包含符号位），在构造后为 (imm[20], imm[10:1], imm[11], imm[19:12], 0)，注意最低位为0，因为J格式的偏移是字对齐。
*/
#define immJ()                                \
  do                                          \
  {                                           \
    *imm = SEXT((BITS(i, 31, 31) << 20) |     \
                    (BITS(i, 30, 21) << 1) |  \
                    (BITS(i, 20, 20) << 11) | \
                    (BITS(i, 19, 12) << 12),  \
                21);                          \
  } while (0)

static void decode_operand(Decode *s, int *rd, word_t *src1, word_t *src2, word_t *imm, int type)
{
  uint32_t i = s->isa.inst;
  int rs1 = BITS(i, 19, 15);
  int rs2 = BITS(i, 24, 20);
  *rd = BITS(i, 11, 7);
  switch (type)
  {
  case TYPE_I:
    src1R();
    immI();
    break;
  case TYPE_U:
    immU();
    break;
  case TYPE_S:
    src1R();
    src2R();
    immS();
    break;
  case TYPE_R:
    src1R();
    src2R();
    break;
  case TYPE_B:
    src1R();
    src2R();
    immB();
    break;
  case TYPE_J:
    immJ();
    break;
  case TYPE_N:
    break;
  default:
    panic("unsupported type = %d", type);
  }
}

static int decode_exec(Decode *s)
{
  s->dnpc = s->snpc;

#define INSTPAT_INST(s) ((s)->isa.inst)
#define INSTPAT_MATCH(s, name, type, ... /* execute body */)         \
  {                                                                  \
    int rd = 0;                                                      \
    word_t src1 = 0, src2 = 0, imm = 0;                              \
    decode_operand(s, &rd, &src1, &src2, &imm, concat(TYPE_, type)); \
    __VA_ARGS__;                                                     \
  }

  INSTPAT_START();

  // RV32I Base Integer Instructions
  INSTPAT("??????? ????? ????? ??? ????? 00101 11", auipc, U, R(rd) = s->pc + imm);
  INSTPAT("??????? ????? ????? ??? ????? 01101 11", lui, U, R(rd) = imm);

  INSTPAT("??????? ????? ????? 000 ????? 00000 11", lb, I, R(rd) = (int8_t)Mr(src1 + imm, 1));
  INSTPAT("??????? ????? ????? 001 ????? 00000 11", lh, I, R(rd) = (int16_t)Mr(src1 + imm, 2));
  INSTPAT("??????? ????? ????? 010 ????? 00000 11", lw, I, R(rd) = (int32_t)Mr(src1 + imm, 4));
  INSTPAT("??????? ????? ????? 100 ????? 00000 11", lbu, I, R(rd) = (uint8_t)Mr(src1 + imm, 1));
  INSTPAT("??????? ????? ????? 101 ????? 00000 11", lhu, I, R(rd) = (uint16_t)Mr(src1 + imm, 2));

  INSTPAT("??????? ????? ????? 000 ????? 01000 11", sb, S, Mw(src1 + imm, 1, BITS(src2, 7, 0)));
  INSTPAT("??????? ????? ????? 001 ????? 01000 11", sh, S, Mw(src1 + imm, 2, BITS(src2, 15, 0)));
  INSTPAT("??????? ????? ????? 010 ????? 01000 11", sw, S, Mw(src1 + imm, 4, src2));

  INSTPAT("0000000 ????? ????? 000 ????? 01100 11", add, R, R(rd) = src1 + src2);
  INSTPAT("0100000 ????? ????? 000 ????? 01100 11", sub, R, R(rd) = src1 - src2);
  INSTPAT("0000000 ????? ????? 100 ????? 01100 11", xor, R, R(rd) = src1 ^ src2);
  INSTPAT("0000000 ????? ????? 110 ????? 01100 11", or, R, R(rd) = src1 | src2);
  INSTPAT("0000000 ????? ????? 111 ????? 01100 11", and, R, R(rd) = src1 & src2);

  INSTPAT("0000000 ????? ????? 001 ????? 0110011", sll, R, R(rd) = (uint32_t)src1 << (src2 & 0x1F));
  INSTPAT("0000000 ????? ????? 101 ????? 0110011", srl, R, R(rd) = ((uint32_t)src1) >> (src2 & 0x1F));
  INSTPAT("0100000 ????? ????? 101 ????? 0110011", sra, R, R(rd) = ((int32_t)src1) >> (src2 & 0x1F));
  INSTPAT("0000000 ????? ????? 010 ????? 0110011", slt, R, R(rd) = ((int32_t)src1 < (int32_t)src2) ? 1 : 0);
  INSTPAT("0000000 ????? ????? 011 ????? 0110011", sltu, R, R(rd) = ((uint32_t)src1 < (uint32_t)src2) ? 1 : 0);

  // RV32M Multiply/Divide Extension Instructions
  // 0930 13:33 through tests,i find mulh and mulhsu do something wrong,but else 6 inst are correct .so i fix the code.in 13:34.
  INSTPAT("0000001 ????? ????? 000 ????? 0110011", mul, R, R(rd) = BITS(((int64_t)src1 * (int64_t)src2), 31, 0));
  INSTPAT("0000001 ????? ????? 001 ????? 0110011", mulh, R, R(rd) = mulh_helper(src1, src2));
  INSTPAT("0000001 ????? ????? 010 ????? 0110011", mulhsu, R, R(rd) = mulhsu_helper(src1, src2));

  INSTPAT("0000001 ????? ????? 011 ????? 0110011", mulhu, R, R(rd) = BITS(((uint64_t)src1 * (uint64_t)src2), 63, 32));
  INSTPAT("0000001 ????? ????? 100 ????? 0110011", div, R, R(rd) = (src2 != 0) ? ((int32_t)src1 / (int32_t)src2) : -1);
  INSTPAT("0000001 ????? ????? 101 ????? 0110011", divu, R, R(rd) = (src2 != 0) ? ((uint32_t)src1 / (uint32_t)src2) : -1);
  INSTPAT("0000001 ????? ????? 110 ????? 0110011", rem, R, R(rd) = (src2 != 0) ? ((int32_t)src1 % (int32_t)src2) : src1);
  INSTPAT("0000001 ????? ????? 111 ????? 0110011", remu, R, R(rd) = (src2 != 0) ? ((uint32_t)src1 % (uint32_t)src2) : src1);

  // Additional RV32I Instructions
  INSTPAT("??????? ????? ????? 000 ????? 00100 11", addi, I, R(rd) = src1 + imm);
  INSTPAT("??????? ????? ????? 100 ????? 00100 11", xori, I, R(rd) = src1 ^ imm);
  INSTPAT("??????? ????? ????? 110 ????? 00100 11", ori, I, R(rd) = src1 | imm);
  INSTPAT("??????? ????? ????? 111 ????? 00100 11", andi, I, R(rd) = src1 & imm);
  INSTPAT("0000000 ????? ????? 001 ????? 0010011", slli, I, R(rd) = src1 << (imm & 0x1F));
  INSTPAT("0000000 ????? ????? 101 ????? 0010011", srli, I, R(rd) = (uint32_t)src1 >> (imm & 0x1F));
  INSTPAT("0100000 ????? ????? 101 ????? 0010011", srai, I, R(rd) = (int32_t)src1 >> (imm & 0x1F));
  INSTPAT("??????? ????? ????? 010 ????? 0010011", slti, I, R(rd) = ((int32_t)src1 < (int32_t)imm) ? 1 : 0);
  INSTPAT("??????? ????? ????? 011 ????? 0010011", sltiu, I, R(rd) = ((uint32_t)src1 < (uint32_t)imm) ? 1 : 0);

  INSTPAT("??????? ????? ????? 000 ????? 1100011", beq, B, s->dnpc = (src1 == src2) ? s->pc + imm : s->snpc);
  INSTPAT("??????? ????? ????? 001 ????? 1100011", bne, B, s->dnpc = (src1 != src2) ? s->pc + imm : s->snpc);
  INSTPAT("??????? ????? ????? 100 ????? 1100011", blt, B, s->dnpc = ((int32_t)src1 < (int32_t)src2) ? s->pc + imm : s->snpc);
  INSTPAT("??????? ????? ????? 101 ????? 1100011", bge, B, s->dnpc = ((int32_t)src1 >= (int32_t)src2) ? s->pc + imm : s->snpc);
  INSTPAT("??????? ????? ????? 110 ????? 1100011", bltu, B, s->dnpc = ((uint32_t)src1 < (uint32_t)src2) ? s->pc + imm : s->snpc);
  INSTPAT("??????? ????? ????? 111 ????? 1100011", bgeu, B, s->dnpc = ((uint32_t)src1 >= (uint32_t)src2) ? s->pc + imm : s->snpc);
  INSTPAT("??????? ????? ????? ??? ????? 1101111", jal, J, R(rd) = s->pc + 4; s->dnpc = s->pc + imm);
  INSTPAT("??????? ????? ????? 000 ????? 1100111", jalr, I, { word_t target = (src1 + imm) & ~1; R(rd) = s->pc + 4; s->dnpc = target; });

  // Trap handling and default case
  INSTPAT("0000000 00001 00000 000 00000 11100 11", ebreak, N, NEMUTRAP(s->pc, R(10))); // R(10) is $a0
  INSTPAT("??????? ????? ????? ??? ????? ????? ??", inv, N, INV(s->pc));

  INSTPAT_END();

  R(0) = 0; // reset $zero to 0

  return 0;
}

int isa_exec_once(Decode *s)
{
  s->isa.inst = inst_fetch(&s->snpc, 4);
  return decode_exec(s);
}