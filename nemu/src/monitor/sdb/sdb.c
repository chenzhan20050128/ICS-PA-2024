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

#include <isa.h>
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "sdb.h"
#include <memory/vaddr.h>

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char *rl_gets()
{
  static char *line_read = NULL;

  if (line_read)
  {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read)
  {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args)
{
  cpu_exec(-1);
  return 0;
}

static int cmd_q(char *args)
{
  return -1;
}

static int cmd_si(char *args)
{
  char *arg = strtok(NULL, " ");
  if (arg == NULL)
  { // 当N没有给出时, 缺省为1
    cpu_exec(1);
    return 0;
  }
  int num = atoi(arg);
  if (num == 0 && strcmp(arg, "0") != 0)
  {
    Assert(0, "Error:invalid num:\n");
  }
  else
  {
    cpu_exec(num);
  }
  return 0;
}
static int cmd_info(char *args)
{
  char *arg = strtok(NULL, " ");
  if (strcmp(arg, "r") == 0)
  {
    isa_reg_display();
  }
  else if (strcmp(arg, "w") == 0)
  {
    print_watchpoints();
  }

  return 0;
}
static int cmd_x(char *args)
{
  char *arg = strtok(NULL, " ");
  // x N EXPR
  int repeat_num = atoi(arg);
  arg = strtok(NULL, " ");
  bool success;
  word_t address = expr(arg, &success);
  if (success)
  {
    for (int i = 0; i < repeat_num; i++)
    {
      word_t mem_addr = address + i * 4;
      word_t value = vaddr_read(mem_addr, 4);

      // 打印地址和内容
      printf("0x%08x: ", mem_addr);
      for (int j = 0; j < 4; j++)
      {
        printf("0x%02x    ", (value >> (j * 8)) & 0xFF);
      }
      printf("\n");
    }
  }
  else
  {
    printf("Unknown expressions '%s'\n", arg);
    return -1;
  }
  return 0;
}
// 表达式求值	p EXPR	p $eax + 1	求出表达式EXPR的值, EXPR支持的
// 运算请见调试中的表达式求值小节
// 设置监视点	w EXPR	w *0x2000	当表达式EXPR的值发生变化时, 暂停程序执行
// 删除监视点	d N	d 2	删除序号为N的监视点
static int cmd_p(char *args)
{
  bool success;
  word_t result = expr(args, &success);
  if (success)
  {
    printf("%u\n", result);
  }
  else
  {
    Assert(0, "Error:cmd_p");
  }
  return 0;
}
static int cmd_w(char *args)
{
  if (args == NULL)
  {
    Assert(0, "Error:cmd_w needs argument:expression char*");
  }
  else
  {
    new_wp(args);
  }
  return 0;
}
static int cmd_d(char *args)
{
  char *arg = strtok(NULL, " ");
  if (arg == NULL)
  {
    Assert(0, "Error:cmd_d needs argument:N int");
  }
  int num = atoi(arg);
  if (num == 0 && strcmp(arg, "0") != 0)
  {
    Assert(0, "Error:invalid num:N\n");
  }
  else
  {
    delete_watchpoint(num);
  }
  return 0;
}

static int cmd_help(char *args);

static struct
{
  const char *name;
  const char *description;
  int (*handler)(char *);
} cmd_table[] = {
    {"help", "Display information about all supported commands", cmd_help},
    {"c", "Continue the execution of the program", cmd_c},
    {"q", "Exit NEMU", cmd_q},
    {"si", "Pause execution after stepping through N instructions", cmd_si},
    {"info", "print massage in registers or watchpoints", cmd_info},
    {"x", "Calculate EXPR, output N consecutive 4-byte values hex.", cmd_x},
    {"p", "Calculate value of EXPR", cmd_p},
    {"w", "Pause program execution when the value of expression EXPR changes", cmd_w},
    {"d", "Delete the watchpoint at index N", cmd_d}

    /* TODO: Add more commands */
    /*Done in 09221853*/

};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args)
{
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL)
  {
    /* no argument given */
    for (i = 0; i < NR_CMD; i++)
    {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else
  {
    for (i = 0; i < NR_CMD; i++)
    {
      if (strcmp(arg, cmd_table[i].name) == 0)
      {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void sdb_set_batch_mode()
{
  is_batch_mode = true;
}

void sdb_mainloop()
{
  if (is_batch_mode)
  {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL;)
  {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL)
    {
      continue;
    }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end)
    {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i++)
    {
      if (strcmp(cmd, cmd_table[i].name) == 0)
      {
        if (cmd_table[i].handler(args) < 0)
        {
          return;
        }
        break;
      }
    }

    if (i == NR_CMD)
    {
      printf("Unknown command '%s'\n", cmd);
    }
  }
}

void init_sdb()
{
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
