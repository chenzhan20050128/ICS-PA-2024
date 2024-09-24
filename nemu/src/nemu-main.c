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

#include <common.h>

// below #include are add by cz 0924 19:05
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <sdb.h>
#include <isa.h>
#include <memory/vaddr.h>

void init_monitor(int, char *[]);
void am_init_monitor();
void engine_start();
int is_exit_status_bad();
void test_main();

int main(int argc, char *argv[])
{
  /* Initialize the monitor. */
#ifdef CONFIG_TARGET_AM
  am_init_monitor();
#else
  init_monitor(argc, argv);
#endif

  /* Start engine. */
  engine_start();

  test_main();

  return is_exit_status_bad();
}

void test_main()
{
  FILE *input_file = fopen("/home/chenzhan/Desktop/ics2024/nemu/tools/gen-expr/input_expression.txt", "r");
  FILE *output_file = fopen("/home/chenzhan/Desktop/ics2024/nemu/tools/gen-expr/output_expression.txt", "w");

  if (input_file == NULL)
  {
    fprintf(stderr, "Failed to open input file\n");
    exit(EXIT_FAILURE);
  }

  assert(output_file != NULL);

  printf("Reading input file...\n");
  int num_tests = 0;

  if (fscanf(input_file, "%d", &num_tests) != 1)
  {
    fprintf(stderr, "Failed to read number of tests\n");
    fclose(input_file);
    exit(EXIT_FAILURE);
  }

  fprintf(output_file, "num_tests:%d\n", num_tests);

  init_regex();

  for (int i = 0; i < num_tests; i++)
  {
    uint32_t expected_result;
    char expression[65536];

    // 读取期望值和表达式
    if (fscanf(input_file, "%u %[^\n]s", &expected_result, expression) != 2)
    {
      fprintf(stderr, "Failed to read expected result and expression\n");
      fclose(input_file);
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
  printf("Test finished\n");
  fclose(input_file);
  fclose(output_file);
  return;
}
