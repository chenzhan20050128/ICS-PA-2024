#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>
#include <limits.h>

static char buf[65536] = {};
static char code_buf[65536 + 128] = {}; // 为 C 代码缓冲区预留更多空间
static char *code_format =
    "#include <stdio.h>\n"
    "int main() { "
    "  unsigned result = %s; "
    "  printf(\"%%u\", result); "
    "  return 0; "
    "}";

static int num_parentheses = 0; // 记录括号的数量，保证括号匹配

#define EXPR_MAX_LENGTH 10

static char *gen_rand_subexpr();

static void ensure_no_div_zero(char *subexpr)
{
  while (strstr(subexpr, "/ 0") != NULL || strstr(subexpr, "/0") != NULL)
  {
    free(subexpr);
    subexpr = gen_rand_subexpr(EXPR_MAX_LENGTH);
  }
}

static char *gen_rand_subexpr(int maxLength)
{
  char *subexpr = malloc(65536 * sizeof(char));
  subexpr[0] = '\0';
  int choice;
  if (maxLength <= 1)
  {
    choice = rand() % 2;
  }
  else
  {
    if (maxLength == EXPR_MAX_LENGTH)
    {
      choice = 2; // not allow the single num,to make the expression length longer.(cz)
    }
    else
    {
      choice = rand() % 3;
    }
  }

  char *temp;

  switch (choice)
  {
  case 0:
  {
    char num_buf[100];
    unsigned num = rand() % 10; // 生成 0 到 9 的随机无符号数
    sprintf(num_buf, "%u", num);
    if (rand() % 6 == 0)
    {
      strcat(subexpr, "(-");
      strcat(subexpr, num_buf);
      strcat(subexpr, ")");
    }
    else
    {
      strcat(subexpr, num_buf);
    }

    break;
  }
  case 1:
  {
    temp = gen_rand_subexpr(maxLength);
    int len = strlen(temp);
    if (temp[0] == '(' && temp[len - 1] == ')')
    {
      // already has an () so won't add () again 0924 19:50 (cz)
      strcat(subexpr, temp);
      free(temp);
    }
    else
    {
      strcat(subexpr, "(");
      num_parentheses++;
      strcat(subexpr, temp);
      free(temp);
      strcat(subexpr, ")");
      num_parentheses--;
    }
    break;
  }
  default:
  {
    char *left = gen_rand_subexpr(maxLength / 2);
    strcat(subexpr, left);
    free(left);

    if (rand() % 2 == 0)
    {
      strcat(subexpr, " ");
    }

    char op_buf[4];
    char op = "+-*/"[rand() % 4];
    sprintf(op_buf, "%c", op);
    strcat(subexpr, op_buf);

    if (rand() % 2 == 0)
    {
      strcat(subexpr, " ");
    }

    char *right = gen_rand_subexpr(maxLength / 2);

    while (op == '/' && strcmp(right, "0") == 0)
    {
      free(right);
      right = gen_rand_subexpr(maxLength / 2);
    }

    strcat(subexpr, right);
    free(right);

    break;
  }
  }

  ensure_no_div_zero(subexpr);
  return subexpr;
}

static int try_eval_expr()
{
  sprintf(code_buf, code_format, buf);

  FILE *fp = fopen("/tmp/.code.c", "w");
  assert(fp != NULL);
  fputs(code_buf, fp);
  fclose(fp);

  int ret = system("gcc /tmp/.code.c -o /tmp/.expr");
  if (ret != 0)
    return -1;

  fp = popen("/tmp/.expr", "r");
  assert(fp != NULL);

  unsigned result;
  ret = fscanf(fp, "%u", &result);
  pclose(fp);

  if (ret != 1)
    return -1;

  return result;
}

static void gen_rand_expr()
{
  int result;
  do
  {
    char *subexpr = gen_rand_subexpr(EXPR_MAX_LENGTH);
    strncpy(buf, subexpr, 65536);
    free(subexpr);

    while (num_parentheses > 0)
    {
      strcat(buf, ")");
      num_parentheses--;
    }

    result = try_eval_expr();
  } while (result == -1);

  FILE *output_file = fopen("input_expression.txt", "a");
  assert(output_file != NULL);
  fprintf(output_file, "%u %s\n", result, buf);
  fclose(output_file);

  // printf("%u %s\n", result, buf);
}

int main(int argc, char *argv[])
{
  int seed = time(0);
  srand(seed);
  int loop = 1; // 默认循环次数
  if (argc > 1)
  {
    sscanf(argv[1], "%d", &loop);
  }

  // print the loop in input_expression.txt cz 0923 13:47
  FILE *output_file = fopen("input_expression.txt", "w");
  assert(output_file != NULL);
  fprintf(output_file, "%d\n", loop);
  fclose(output_file);

  for (int i = 0; i < loop; i++)
  {
    if (i % 100 == 0)
    {
      printf("Generate i'th input expression:%d\n", i);
    }
    gen_rand_expr(); // 生成随机表达式
  }
  printf("Generate input expressions finished\n");

  return 0;
}