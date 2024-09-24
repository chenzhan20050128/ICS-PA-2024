/***************************************************************************************
 * Copyright (c) 2014-2024 Zihao Yu, Nanjing University
 *
 * NEMU is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 *
 * See the Mulan PSL v2 for more details.
 ***************************************************************************************/

// #define Log(fmt, ...) printf(fmt "\n", ##__VA_ARGS__)
#include <regex.h>
// #include "common.h" //0924 17:25 cz
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <sdb.h>
#include <isa.h>
#include <memory/vaddr.h>
#include <common.h>
// #include "/home/chenzhan/Desktop/ics2024/nemu/src/monitor/sdb/sdb.h"
// #include "/home/chenzhan/Desktop/ics2024/nemu/include/isa.h"
// #include "/home/chenzhan/Desktop/ics2024/nemu/include/common.h"
// #include "/home/chenzhan/Desktop/ics2024/nemu/include/memory/vaddr.h"

typedef unsigned int uint32_t;

enum
{
  TK_NOTYPE = 256, // 空格串
  TK_EQ,           // 双等号
  TK_LT,           // 小于
  TK_LE,           // 小于等于
  TK_GT,           // 大于
  TK_GE,           // 大于等于
  TK_NEQ,          // 不等于

  TK_NUM, // 十进制整数
  TK_HEX, // 十六进制数
  TK_REG, // 寄存器

  /* TODO: Add more token types */
  TK_ADD,           // 加号
  TK_MINUS,         // 减号
  TK_MULTIPLE,      // 乘号
  TK_DIVIDE,        // 除号
  TK_LEFT_BRACKET,  // 左括号
  TK_RIGHT_BRACKET, // 右括号
  TK_DEREF,         // 指针解引用
  TK_NEGATIVE

};

static struct rule
{
  const char *regex;
  int token_type;
} rules[] = {

    /* TODO: Add more rules.
     * Pay attention to the precedence level of different rules.
     */
    {" +", TK_NOTYPE},                     // 空格串
    {"==", TK_EQ},                         // 双等号
    {"<=", TK_LE},                         // 小于等于
    {"<", TK_LT},                          // 小于
    {">=", TK_GE},                         // 大于等于
    {">", TK_GT},                          // 大于
    {"!=", TK_NEQ},                        // 不等于
    {"\\+", TK_ADD},                       // 加号
    {"-", TK_MINUS},                       // 减号
    {"\\*", TK_MULTIPLE},                  // 乘号
    {"/", TK_DIVIDE},                      // 除号
    {"\\(", TK_LEFT_BRACKET},              // 左括号
    {"\\)", TK_RIGHT_BRACKET},             // 右括号
    {"0x[0-9a-fA-F]+", TK_HEX},            // 十六进制数
    {"\\$[a-zA-Z_][a-zA-Z0-9_]*", TK_REG}, // 寄存器
    {"[0-9]+", TK_NUM},                    // 十进制整数
}; // notice ! there are no TK_DEREF! because it is same as TK_MULTIPLE literally!there are also no TK_NEGATIVE

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]))
#define TOKENS_NUM 10010 // cz 0923 14:27
static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex()
{
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i++)
  {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0)
    {
      regerror(ret, &re[i], error_msg, 128);
      printf("regex compilation failed: %s\n%s\n", error_msg, rules[i].regex);
      exit(1);
    }
  }
}

typedef struct token
{
  int type;
  char str[32];
} Token;

static Token tokens[TOKENS_NUM] __attribute__((used)) = {};
static int nr_token __attribute__((used)) = 0;

static bool make_token(char *e)
{
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0')
  {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i++)
    {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0)
      {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        position += substr_len;

        /* 记录 token 到 tokens 数组 */
        switch (rules[i].token_type)
        {
        case TK_NOTYPE:
          /* 忽略空白字符 */
          break;
        case TK_NUM:
        case TK_HEX:
        case TK_REG:
          if (nr_token >= TOKENS_NUM)
          {
            printf("Error: too many tokens\n");
            return false;
          }
          tokens[nr_token].type = rules[i].token_type;
          if (substr_len < sizeof(tokens[nr_token].str))
          {
            strncpy(tokens[nr_token].str, substr_start, substr_len);
            tokens[nr_token].str[substr_len] = '\0'; // 添加字符串终止符
          }
          else
          {
            strncpy(tokens[nr_token].str, substr_start, sizeof(tokens[nr_token].str) - 1);
            tokens[nr_token].str[sizeof(tokens[nr_token].str) - 1] = '\0';
          }
          nr_token++;
          break;
        case TK_ADD:
        case TK_MINUS:
        case TK_MULTIPLE:
        case TK_DIVIDE:
        case TK_LEFT_BRACKET:
        case TK_RIGHT_BRACKET:
        case TK_EQ:
        case TK_NEQ:
        case TK_GE:
        case TK_GT:
        case TK_LE:
        case TK_LT:
          if (nr_token >= TOKENS_NUM)
          {
            printf("Error: too many tokens\n");
            return false;
          }
          tokens[nr_token].type = rules[i].token_type;
          if (substr_len < sizeof(tokens[nr_token].str))
          {
            strncpy(tokens[nr_token].str, substr_start, substr_len);
            tokens[nr_token].str[substr_len] = '\0';
          }
          else
          {
            strncpy(tokens[nr_token].str, substr_start, sizeof(tokens[nr_token].str) - 1);
            tokens[nr_token].str[sizeof(tokens[nr_token].str) - 1] = '\0';
          }
          nr_token++;
          break;
        default:
          /* 处理未知的 token 类型 */
          printf("Unknown token type: %d\n", rules[i].token_type);
          return false;
        }

        break;
      }
    }

    if (i == NR_REGEX)
    {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

static bool check_parentheses(int p, int q)
{
  if (tokens[p].type != TK_LEFT_BRACKET || tokens[q].type != TK_RIGHT_BRACKET)
    return false;

  int stack = 0;
  for (int i = p; i <= q; i++)
  {
    if (tokens[i].type == TK_LEFT_BRACKET)
      stack++;
    else if (tokens[i].type == TK_RIGHT_BRACKET)
      stack--;

    if (stack == 0 && i < q)
      return false;
  }

  return stack == 0;
}

static int find_main_operator(int p, int q)
{
  int min_precedence = 1000;
  int main_op = -1;
  int stack = 0;

  /* 定义运算符的优先级
   * 优先级从低到高: == (0) < +,- (1) < *,/ (2)
   */
  int precedence[] = {
      [TK_EQ] = 0,
      [TK_NEQ] = 0,
      [TK_LE] = 0,
      [TK_LT] = 0,
      [TK_GE] = 0,
      [TK_GT] = 0,
      [TK_ADD] = 1,
      [TK_MINUS] = 1,
      [TK_MULTIPLE] = 2,
      [TK_DIVIDE] = 2,
      [TK_DEREF] = 3,
      [TK_NEGATIVE] = 4};

  for (int i = p; i <= q; i++)
  {
    if (tokens[i].type == TK_LEFT_BRACKET)
    {
      stack++;
      continue;
    }
    if (tokens[i].type == TK_RIGHT_BRACKET)
    {
      stack--;
      continue;
    }
    if (stack == 0 && tokens[i].type != TK_NUM && tokens[i].type != TK_HEX && tokens[i].type != TK_REG)
    {
      int current_precedence = precedence[tokens[i].type];
      if (current_precedence <= min_precedence)
      {
        min_precedence = current_precedence;
        main_op = i;
      }
    }
  }

  return main_op;
}

static uint32_t eval(int p, int q, bool *success)
{
  if (p > q)
  {
    /* 错误的表达式 */
    *success = false;
    return 0;
  }
  else if (p == q)
  {
    /* 单个 token，应该是一个数字、十六进制数或寄存器 */
    if (tokens[p].type == TK_NUM)
    {
      return strtoul(tokens[p].str, NULL, 10);
    }
    else if (tokens[p].type == TK_HEX)
    {
      return strtoul(tokens[p].str + 2, NULL, 16); // 跳过"0x"
    }
    else if (tokens[p].type == TK_REG)
    {
      bool success_reg;
      uint32_t reg_val = isa_reg_str2val(tokens[p].str + 1, &success_reg); // 跳过"$"
      if (!success_reg)
      {
        *success = false;
        return 0;
      }
      return reg_val;
    }
    *success = false;
    return 0;
  }
  else if (check_parentheses(p, q))
  {
    /* 表达式被括号包围，去掉括号 */
    return eval(p + 1, q - 1, success);
  }
  else
  {
    /* 寻找主运算符 */
    int op = find_main_operator(p, q);
    if (op == -1)
    {
      /* 没有找到主运算符 */
      *success = false;
      return 0;
    }
    uint32_t val1 = eval(p, op - 1, success);
    if (!(*success))
    {
      printf("Error:Val1 don't success\n");
      return 0;
    }
    uint32_t val2 = eval(op + 1, q, success);
    if (!(*success))
    {
      printf("Error:Val2 don't success\n");
      return 0;
    }
    switch (tokens[op].type)
    {
    case TK_ADD:
      return val1 + val2;
    case TK_MINUS:
      return val1 - val2;
    case TK_MULTIPLE:
      return val1 * val2;
    case TK_DIVIDE:
      if (val2 == 0)
      {
        printf("Error: Division by zero\n");
        *success = false;
        return 0;
      }
      return val1 / val2;
    case TK_EQ:
      return val1 == val2;
    case TK_NEQ:
      return val1 != val2;
    case TK_LT:
      return val1 < val2;
    case TK_LE:
      return val1 <= val2;
    case TK_GT:
      return val1 > val2;
    case TK_GE:
      return val1 >= val2;
    case TK_DEREF:
      return vaddr_read(val1, 4); // 32bit riscv should 4B len(cz)
    case TK_NEGATIVE:
      return (-1) * val1;
    default:
      printf("Error: Unknown operator type %d\n", tokens[op].type);
      *success = false;
      return 0;
    }
  }
}

uint32_t expr(char *e, bool *success)
{
  if (!make_token(e))
  {
    Assert(0, "Error:can't make_token");
    *success = false;
    return 0;
  }

  /* 打印 tokens，便于调试 */
  /*
  for (int i = 0; i < nr_token; i++)
  {
    printf("tokens[%d]: type=%d, str=%s\n", i, tokens[i].type, tokens[i].str);
  }
  */

  *success = true;

  for (int i = 0; i < nr_token; i++)
  {
    if (i == 0 || tokens[i - 1].type == TK_LEFT_BRACKET ||
        tokens[i - 1].type == TK_ADD ||
        tokens[i - 1].type == TK_MINUS ||
        tokens[i - 1].type == TK_MULTIPLE ||
        tokens[i - 1].type == TK_DIVIDE ||
        tokens[i - 1].type == TK_DEREF ||
        tokens[i - 1].type == TK_EQ ||
        tokens[i - 1].type == TK_NEQ ||
        tokens[i - 1].type == TK_LT ||
        tokens[i - 1].type == TK_LE ||
        tokens[i - 1].type == TK_GT ||
        tokens[i - 1].type == TK_GE ||
        tokens[i - 1].type == TK_RIGHT_BRACKET)
    {
      if (tokens[i].type == TK_MULTIPLE)
      {
        tokens[i].type = TK_DEREF; // 改为指针解引用类型
      }
      else if (tokens[i].type == TK_MINUS)
      {
        tokens[i].type = TK_NEGATIVE;
      }
      else
      {
      }
    }
  }

  return eval(0, nr_token - 1, success);
}

/* 测试主函数*/
int test_main(int argc, char *argv[])
{
  FILE *output_file = fopen("output_expression.txt", "w");
  assert(output_file != NULL);

  printf("Hello!Please write the answer and expression:\n");
  int num_tests = 0;

  if (scanf("%d", &num_tests) != 1)
  {
    fprintf(stderr, "Failed to read number of tests\n");
    exit(EXIT_FAILURE);
  }

  fprintf(output_file, "num_tests:%d\n", num_tests);

  init_regex();

  for (int i = 0; i < num_tests; i++)
  {
    uint32_t expected_result;
    char expression[65536];
    // 读取期望值和表达式
    if (scanf("%u %[^\n]s", &expected_result, expression) != 2)
    {
      fprintf(stderr, "Failed to read expected result and expression\n");
      exit(EXIT_FAILURE);
    }
    bool success = false;
    uint32_t result = expr(expression, &success);

    if (success && result == expected_result)
    {
      fprintf(output_file, "Test %d: success, Result = %u\n", i + 1, result);
    }
    else
    {
      fprintf(output_file, "Test %d: %s 错误值：%u 正确值：%u\n", i + 1, expression, result, expected_result);
    }
  }

  fclose(output_file);
  return 0;
}
