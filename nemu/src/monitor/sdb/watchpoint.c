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

#include "sdb.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <utils.h>

#define NR_WP 32
typedef unsigned int uint32_t;

/* 监视点池和链表头指针定义为静态，全局仅在此文件内部可见 */
static WP wp_pool[NR_WP] = {};
static WP *head = NULL;
static WP *free_ = NULL;

void init_wp_pool()
{
  int i;
  for (i = 0; i < NR_WP; i++)
  {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1) ? NULL : &wp_pool[i + 1];
    wp_pool[i].expression[0] = '\0';
    wp_pool[i].value = 0;
  }
  head = NULL;
  free_ = wp_pool;
}

WP *new_wp(const char *e)
{
  if (free_ == NULL)
  {
    Assert(0, "No free watchpoints available.");
  }

  WP *wp = free_;
  free_ = free_->next;

  strncpy(wp->expression, e, sizeof(wp->expression) - 1);
  wp->expression[sizeof(wp->expression) - 1] = '\0'; /* 确保字符串以 NULL 结尾 */

  bool success;
  wp->value = expr(wp->expression, &success);
  if (!success)
  {
    /* 表达式求值失败，释放监视点并终止程序 */
    free_wp(wp);
    Assert(0, "Expression evaluation failed:%s\n", wp->expression);
  }

  wp->next = head;
  head = wp;

  return wp;
}

void free_wp(WP *wp)
{

  if (head == wp)
  {
    head = head->next;
  }
  else
  {
    WP *prev = head;
    while (prev != NULL && prev->next != wp)
    {
      prev = prev->next;
    }
    if (prev == NULL)
    {
      /* 监视点未在使用链表中找到，程序异常 */
      Assert(0, "Watchpoint to free not found in the active list.");
    }
    prev->next = wp->next;
  }

  wp->expression[0] = '\0';
  wp->value = 0;

  /* 将监视点插入空闲链表头 */
  wp->next = free_;
  free_ = wp;
}

void print_watchpoints()
{
  WP *wp = head;
  printf("Watchpoint\tExpression\tValue\tHex Value\n");
  while (wp != NULL)
  {
    printf("wp%d\t\t%s\t%u\t0x%x\n", wp->NO, wp->expression, wp->value, wp->value);
    wp = wp->next;
  }
}

/*
 * check watchpoints,if any watchpoint's expression change,it will stop.
 */
void check_watchpoints()
{

  WP *wp = head;
  while (wp != NULL)
  {
    bool success;
    uint32_t new_val = expr(wp->expression, &success);
    if (!success)
    {
      /* 表达式求值失败，忽略该监视点 */
      wp = wp->next;
      continue;
    }
    if (new_val != wp->value)
    {
      /* 监视点触发，更新值并暂停 */
      printf("Watchpoint wp%d triggered: %s\n"
             "Old value: %u\t(0x%x)\n" // also print the hex number
             "New value: %u\t(0x%x)\n",
             wp->NO, wp->expression, wp->value, wp->value, new_val, new_val);
      nemu_state.state = NEMU_STOP;
      wp->value = new_val;
      return;
    }
    wp = wp->next;
  }
}

void delete_watchpoint(int NO)
{
  WP *wp = head;
  while (wp != NULL)
  {
    if (wp->NO == NO)
    {
      free_wp(wp);
      printf("Deleted watchpoint %d\n", NO);
      return;
    }
    wp = wp->next;
  }
  printf("No watchpoint found with number %d\n", NO);
}