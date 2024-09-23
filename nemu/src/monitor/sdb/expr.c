
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

// #include <isa.h> it should be add again (cz)

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#define Log(fmt, ...) printf(fmt "\n", ##__VA_ARGS__)
#include <regex.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

typedef unsigned int uint32_t;

enum
{
  TK_NOTYPE = 256, // 空格串
  TK_EQ,           // 双等号
  TK_NUM,          // 十进制整数

  /* TODO: Add more token types */
  TK_ADD,          // 加号
  TK_MINUS,        // 减号
  TK_MULTIPLE,     // 乘号
  TK_DIVIDE,       // 除号
  TK_LEFT_BRACKET, // 左括号
  TK_RIGHT_BRACKET // 右括号
};

static struct rule
{
  const char *regex;
  int token_type;
} rules[] = {

    /* TODO: Add more rules.
     * Pay attention to the precedence level of different rules.
     */

    {" +", TK_NOTYPE},         // 空格串
    {"==", TK_EQ},             // 双等号
    {"\\+", TK_ADD},           // 加号
    {"-", TK_MINUS},           // 减号
    {"\\*", TK_MULTIPLE},      // 乘号
    {"/", TK_DIVIDE},          // 除号
    {"\\(", TK_LEFT_BRACKET},  // 左括号
    {"\\)", TK_RIGHT_BRACKET}, // 右括号
    {"[0-9]+", TK_NUM},        // 十进制整数
};
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

        // Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
        //   i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* 记录 token 到 tokens 数组 */
        switch (rules[i].token_type)
        {
        case TK_NOTYPE:
          /* 忽略空白字符 */
          break;
        case TK_NUM:
          if (nr_token >= TOKENS_NUM)
          {
            printf("Error: too many tokens\n");
            return false;
          }
          tokens[nr_token].type = TK_NUM;
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
      [TK_ADD] = 1,
      [TK_MINUS] = 1,
      [TK_MULTIPLE] = 2,
      [TK_DIVIDE] = 2};

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
    if (stack == 0 && tokens[i].type != TK_NUM)
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
    /* 单个 token，应该是一个数字 */
    if (tokens[p].type != TK_NUM)
    {
      *success = false;
      return 0;
    }
    return strtoul(tokens[p].str, NULL, 10);
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
  return eval(0, nr_token - 1, success);
}

/* 测试主函数
int main(int argc, char *argv[])
{
  FILE *output_file = fopen("output_expression.txt", "w");
  assert(output_file != NULL);

  int num_tests = 0;
  scanf("%d", &num_tests); // 读取测试的数量
  fprintf(output_file, "num_tests:%d\n", num_tests);

  init_regex();

  for (int i = 0; i < num_tests; i++)
  {
    uint32_t expected_result;
    char expression[65536];
    // 读取期望值和表达式
    scanf("%u %[^\n]s", &expected_result, expression);

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
*/