/*
  This file is part of MAMBO, a low-overhead dynamic binary modification tool:
      https://github.com/beehive-lab/mambo

  Copyright 2013-2016 Cosmin Gorgovan <cosmin at linux-geek dot org>
  Copyright 2017 The University of Manchester

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/
#ifdef PLUGINS_NEW

#include <stdio.h>
#include <assert.h>
#include <locale.h>
#include <inttypes.h>
#include "../plugins.h"
#include "queue.h"
#include "util.h"

unsigned int currentsyscall;

typedef struct bbv_count
{
  unsigned int called;
  unsigned int syscall; // Replaced 'id' with 'syscall'
  struct bbv_count *next;
} bbv_count_t;

void inc_bbv_syscall_in_list(bbv_count_t *head, unsigned int search_syscall)
{
  bbv_count_t *current = head;
  while (current != NULL)
  {
    if (current->syscall == search_syscall)
    {
      current->called++;
      return;
    }
    if (current->next != NULL)
      current = current->next;
    else
      break;
  }
  bbv_count_t *new_bbv_count = (bbv_count_t *)malloc(sizeof(bbv_count_t));
  new_bbv_count->syscall = search_syscall;
  new_bbv_count->called = 1;
  new_bbv_count->next = NULL;
  current->next = new_bbv_count;
}

void print_bbv_list(bbv_count_t *head)
{
  while (head != NULL)
  {
    printf("System call %d called %d times\n", head->syscall, head->called);

    head = head->next;
  }
}

void free_bbv_list(mambo_context *ctx, bbv_count_t *head)
{
  bbv_count_t *last;
  last = head;
  head = head->next;
  mambo_free(ctx, last);

  while (head != NULL)
  {
    last = head;
    head = head->next;
    free(head);
  }
}

void bbv_check_dup(bbv_count_t *head)
{
  bbv_count_t *search;
  while (head != NULL)
  {
    search = head->next;
    while (search != NULL)
    {
      if (head->syscall == search->syscall)
      {
        printf("match for syscall %d called %d times with syscall %d called %d times\n", head->syscall, head->called, search->syscall, search->called);
      }
      search = search->next;
    }
    head = head->next;
  }
}

void bbv_exe(bbv_count_t *bbv_list)
{
  assert(bbv_list != NULL);

  inc_bbv_syscall_in_list(bbv_list, get_svc_type() /*needs to be r8*/);
}

int bbv_hook(mambo_context *ctx)
{

  if (ctx->code.inst == A64_SVC)
  {
    bbv_count_t *bbv_list = (bbv_count_t *)mambo_get_thread_plugin_data(ctx);
    // fprintf(stderr, "bbv_hook called for %d %ls \n", mambo_get_inst(ctx), (uint32_t *)mambo_get_source_addr(ctx));
    emit_push(ctx, (1 << reg0) | (1 << reg1) | (1 << reg2));
    emit_set_reg_ptr(ctx, reg0, bbv_list);
    emit_set_reg(ctx, reg1, mambo_get_inst_type(ctx));
    // emit_set_reg(ctx, reg2, (uintptr_t)mambo_get_source_addr(ctx));
    emit_mov(ctx, 4, 8);
    emit_safe_fcall(ctx, bbv_exe, 1);
    emit_pop(ctx, (1 << reg0) | (1 << reg1) | (1 << reg2));
    return 0;
  }
}
int bbv_pre_thread_handler(mambo_context *ctx)
{
  bbv_count_t *bbv_list = (bbv_count_t *)mambo_alloc(ctx, sizeof(bbv_count_t));
  assert(bbv_list != NULL);
  mambo_set_thread_plugin_data(ctx, bbv_list);
}

int bbv_post_thread_handler(mambo_context *ctx)
{
  bbv_count_t *bbv_list = (bbv_count_t *)mambo_get_thread_plugin_data(ctx);
  print_bbv_list(bbv_list);
  bbv_check_dup(bbv_list);
  free_bbv_list(ctx, bbv_list);
}

int bbv_exit_handler(mambo_context *ctx)
{
  fprintf(stderr, "Exit called:\n");
}

__attribute__((constructor)) void bbv_init_plugin()
{
  mambo_context *ctx = mambo_register_plugin();
  assert(ctx != NULL);
  mambo_register_pre_thread_cb(ctx, &bbv_pre_thread_handler);
  mambo_register_pre_fragment_cb(ctx, &bbv_hook);
  mambo_register_pre_inst_cb(ctx, &bbv_hook);
  mambo_register_post_thread_cb(ctx, &bbv_post_thread_handler);
  mambo_register_exit_cb(ctx, &bbv_exit_handler);
  setlocale(LC_NUMERIC, "");
}
#endif
