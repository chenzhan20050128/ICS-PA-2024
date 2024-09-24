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

#ifndef __SDB_H__
#define __SDB_H__
#include "/home/chenzhan/Desktop/ics2024/nemu/include/common.h"
#include <common.h>
typedef struct watchpoint
{
    int NO;
    struct watchpoint *next;
    char expression[65536];
    uint32_t value;

} WP;
void init_regex();
word_t expr(char *e, bool *success);
void check_watchpoints();
void print_watchpoints();
void delete_watchpoint(int NO);
WP *new_wp(const char *e);
void free_wp(WP *wp);
#endif
