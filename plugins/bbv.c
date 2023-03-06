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

typedef struct bbv_count
{
  unsigned int called;  // number of times the system call is called
  unsigned int syscall; // the system call number
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

// writes to text file as a string
void write_bbv_list_to_string_file(const char *filename, bbv_count_t *head)
{
  printf("Entering write_bbv_to_string_file \n");
  FILE *f = fopen(filename, "w");
  if (!f)
  {
    printf("Error opening file %s for writing.\n", filename);
    return;
  }

  while (head != NULL)
  {
    fprintf(f, "System call %d called %d times\n", head->syscall, head->called);
    head = head->next;
  }

  fclose(f);
  printf("Exiting write_bbv_to_string_file \n");
}

void write_bbv_list_to_file(const char *filename, bbv_count_t *head)
{
  printf("Entering write_bbv_to_file \n");
  FILE *f = fopen(filename, "w+");
  if (!f)
  {
    printf("Error opening file %s for writing.\n", filename);
    return;
  }

  while (head != NULL)
  {
    fprintf(f, "%d \n", head->syscall);
    head = head->next;
  }
  fclose(f);
  printf("Exiting write_bbv_to_file \n");
}

/*// saves the system cals to a file seperated by commas and space
void write_linklist_to_file(bbv_count_t *head, const char *filename)
{
  FILE *f = fopen(filename, "wb");
  if (f == NULL)
  {
    fprintf(stderr, "Error opening file: %s\n", filename);
    return;
  }

  for (bbv_count_t *curr = head; curr != NULL; curr = curr->next)
  {
    fprintf(f, "%d, ", curr->syscall);
  }

  fclose(f);
}*/

bool compare_bbv_to_file(bbv_count_t *head, const char *filename)
{
  printf("Entering compare_bbv_to_file \n");
  FILE *fp = fopen(filename, "r");
  printf("Entering compare_bbv_to_file - File 1 \n");
  if (!fp)
  {
    perror("Error opening file");
    return false;
  }

  int syscall;
  bool found_in_file;
  bool all_matched = true;
  while (fscanf(fp, "%d", &syscall) == 1)
  {
    bbv_count_t *current = head;
    found_in_file = false;
    while (current != NULL)
    {
      if (current->syscall == syscall)
      {
        // printf("Success on sys %d\n", current->syscall);
        found_in_file = true;
        break;
      }
      current = current->next;
    }
    if (!found_in_file)
    {
      printf("System call found in file was not using during runtime:  %d\n", syscall);
      // all_matched = false;
    }
  }
  fclose(fp);
  printf("Closed File 1 in compare_bbv_to_file \n");

  bbv_count_t *current = head;
  while (current != NULL)
  {
    printf("Entering compare_bbv_to_file - File 2 \n");
    found_in_file = false;
    FILE *fp = fopen(filename, "r");
    while (fscanf(fp, "%d", &syscall) == 1)
    {
      if (current->syscall == syscall)
      {
        found_in_file = true;
        break;
      }
    }
    if (!found_in_file)
    {
      printf("failed on system call xxx %d\n", current->syscall);
      all_matched = false;
    }
    current = current->next;
  }
  printf("Exiting compare_bbv_to_file \n");
  return all_matched;
  printf("Returning all_matched in compare_bbv_to_file \n");
}

int check_all_matched(bool all_matched)
{
  if (all_matched)
  {
    printf("Entering IF statement check_all_matched \n");
    printf("All system calls matched\n");
    return 0;
  }
  else
  {
    printf("Entering ELSE statement check_all_matched \n");
    printf("All system calls did not match\n");
    printf("Exiting program check_all_matched \n");
    return 1;
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
    mambo_free(ctx, last);
  }
}

/*void bbv_check_dup(bbv_count_t *head)
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
}*/

void bbv_exe(bbv_count_t *bbv_list)
{
  assert(bbv_list != NULL);
  inc_bbv_syscall_in_list(bbv_list, get_svc_type());
}

int bbv_hook(mambo_context *ctx)
{
  if (ctx->code.inst == A64_SVC)
  {
    bbv_count_t *bbv_list = (bbv_count_t *)mambo_get_thread_plugin_data(ctx);
    // fprintf(stderr, "bbv_hook called for %d %ls \n", mambo_get_inst(ctx), (uint32_t *)mambo_get_source_addr(ctx));
    emit_push(ctx, (1 << reg0) | (1 << reg1) | (1 << reg2));
    emit_set_reg_ptr(ctx, reg0, bbv_list);
    // emit_set_reg(ctx, reg1, mambo_get_inst_type(ctx));
    // raise(SIGTRAP);
    // emit_set_reg(ctx, reg2, (uintptr_t)mambo_get_source_addr(ctx));
    emit_mov(ctx, 1, 8);
    // raise(SIGTRAP);
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
  check_all_matched(compare_bbv_to_file(bbv_list, "whitelist.txt"));
  write_bbv_list_to_file("syscall_list_runtime.txt", bbv_list);
  write_bbv_list_to_string_file("output.txt", bbv_list);
  print_bbv_list(bbv_list);
  // bbv_check_dup(bbv_list);
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
  // mambo_register_pre_fragment_cb(ctx, &bbv_hook);
  mambo_register_pre_inst_cb(ctx, &bbv_hook);
  mambo_register_post_thread_cb(ctx, &bbv_post_thread_handler);
  mambo_register_exit_cb(ctx, &bbv_exit_handler);
  setlocale(LC_NUMERIC, "");
}

#endif

// print off syscall as it is happening
// create a dashboard