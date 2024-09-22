#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>

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

static char *gen_rand_subexpr()
{
  char *subexpr = malloc(65536 * sizeof(char));
  subexpr[0] = '\0';
  int choice = rand() % 3; // 选择 0, 1 或 2
  char *temp;
  // 选择生成数字、括号或运算符
  switch (choice)
  {
  case 0:
  {
    // 生成一个数字
    char num_buf[10];
    sprintf(num_buf, "%u", rand() % 10); // 生成 0 到 9 的随机数
    strcat(subexpr, num_buf);
    break;
  }
  case 1:
  {
    // 生成带括号的表达式，确保括号匹配
    strcat(subexpr, "(");
    num_parentheses++; // 左括号增加
    temp = gen_rand_subexpr();
    strcat(subexpr, temp); // 生成递归表达式
    free(temp);
    strcat(subexpr, ")");
    num_parentheses--; // 右括号减少
    break;
  }
  default:
  {
    // 生成二元运算的表达式
    temp = gen_rand_subexpr();
    strcat(subexpr, temp); // 生成递归表达式
    free(temp);
    if (rand() % 4 == 0)
    {
      strcat(subexpr, " ");
    }
    char op_buf[4];
    sprintf(op_buf, "%c", "+-*"[rand() % 3]); // 随机选择 +, -, *
    strcat(subexpr, op_buf);
    if (rand() % 4 == 0)
    {
      strcat(subexpr, " ");
    }
    temp = gen_rand_subexpr();
    strcat(subexpr, temp); // 生成递归表达式
    free(temp);
    break;
  }
  }
  return subexpr;
}

static void gen_rand_expr()
{
  char *subexpr = gen_rand_subexpr();
  strncpy(buf, subexpr, 65536);
  free(subexpr);
}

int main(int argc, char *argv[])
{
  int seed = time(0);
  srand(seed);  // 初始化随机种子
  int loop = 1; // 默认循环次数
  if (argc > 1)
  {
    sscanf(argv[1], "%d", &loop);
  }
  for (int i = 0; i < loop; i++)
  {
    gen_rand_expr(); // 生成随机表达式

    // 如果有未闭合的括号，补齐右括号
    while (num_parentheses > 0)
    {
      strcat(buf, ")");
      num_parentheses--;
    }

    sprintf(code_buf, code_format, buf); // 格式化代码

    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    int ret = system("gcc /tmp/.code.c -o /tmp/.expr"); // 编译 C 代码
    if (ret != 0)
      continue;

    fp = popen("/tmp/.expr", "r"); // 执行生成的程序
    assert(fp != NULL);

    int result;
    ret = fscanf(fp, "%d", &result); // 获取计算结果
    pclose(fp);

    printf("%u %s\n", result, buf); // 输出结果和表达式
  }
  return 0;
}
// 20240922 21:53 finish generating the expr
// but it could overflow the int
