/*
  This file is part of MAMBO, a low-overhead dynamic binary modification tool:
      https://github.com/beehive-lab/mambo


    Copyright (C) 2023 Muhammad Shirazul Haque - Liverpool John Moores University

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
#include <stdlib.h>
#include <curl/curl.h>

// define a struct to hold the system call number and the number of times it is called
typedef struct scd_count
{
  unsigned int called;    // number of times the system call is called
  unsigned int syscall;   // the system call number
  struct scd_count *next; // pointer to the next node
} scd_count_t;

//// CURL FUNCTION ////
// implement a POST function that uses curl to post the data to a localhost server
int curl_post()
{
  CURL *curl;
  CURLcode res;
  FILE *fp;

  // Open the file for reading
  fp = fopen("syscall_list_runtime.txt", "r");
  if (!fp)
  {
    printf("Error opening file\n");
    return 1;
  }

  // Get the size of the file
  fseek(fp, 0L, SEEK_END);
  long int file_size = ftell(fp);
  rewind(fp);

  // Allocate a buffer to hold the file contents
  char *buffer = (char *)malloc(sizeof(char) * file_size);

  // Read the file into the buffer
  size_t elements_read = fread(buffer, sizeof(char), file_size, fp);
  if (elements_read != file_size)
  {
    fprintf(stderr, "Error reading file\n");
    return 1;
  }

  // Initialize curl
  curl = curl_easy_init();
  if (curl)
  {
    struct curl_slist *headers = NULL;
    // Set the URL, POST fields and headers for the request
    headers = curl_slist_append(headers, "Content-Type: text/plain");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    // Set the URL
    curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:3001/api/data/saveSyscallData");
    // Set the POST method
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    // Set the buffer as the request body
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, buffer);
    // Perform the request
    res = curl_easy_perform(curl);

    // Check for errors
    if (res != CURLE_OK)
    {
      fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    }

    // Cleanup
    curl_easy_cleanup(curl);
  }

  // Close the file and free the buffer
  fclose(fp);
  free(buffer);

  return 0;
}

// function to add a new node to the linked list of system calls
// if the system call is already in the list, increment the number of times it is called
// if the system call is not in the list, add it to the list
void inc_scd_syscall_in_list(scd_count_t *head, unsigned int search_syscall)
{
  if (head == NULL)
  {
    printf("Error: head is NULL in inc_scd_syscall_in_list \n");
    return;
  }

  scd_count_t *current = head;
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
  scd_count_t *new_scd_count = (scd_count_t *)malloc(sizeof(scd_count_t));
  new_scd_count->syscall = search_syscall;
  new_scd_count->called = 1;
  new_scd_count->next = NULL;
  current->next = new_scd_count;
}

// Function to print the linked list of system calls and the number of times they are called
void print_scd_list(scd_count_t *head)
{
  while (head != NULL)
  {
    printf("System call %d called %d times\n", head->syscall, head->called);

    head = head->next;
  }
}

// writes to text file as a string
void write_scd_list_to_string_file(const char *filename, scd_count_t *head)
{
  // printf("Entering write_scd_to_string_file \n");
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
  // printf("Exiting write_scd_to_string_file \n");
}

// writes to text file as a file
void write_scd_list_to_file(const char *filename, scd_count_t *head)
{
  // printf("Entering write_scd_to_file \n");
  FILE *f = fopen(filename, "w+");
  if (!f)
  {
    printf("Error opening file %s for writing.\n", filename);
    return;
  }

  while (head != NULL)
  {
    fprintf(f, "%d, %d\n", head->syscall, head->called);
    head = head->next;
  }
  fclose(f);
  // printf("Exiting write_scd_to_file \n");
}

bool compare_scd_to_file(scd_count_t *head, const char *filename)
{
  // printf("Entering compare_scd_to_file \n");
  FILE *fp = fopen(filename, "r");
  // printf("Entering compare_scd_to_file - File 1 \n");

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
    scd_count_t *current = head;
    found_in_file = false;
    while (current != NULL)
    {
      if (current->syscall == syscall)
      {
        found_in_file = true;
        break;
      }
      current = current->next;
    }
    if (!found_in_file)
    {
      printf("System call found in file was not used during runtime:  %d\n", syscall);
    }
  }
  fclose(fp);
  // printf("Closed File 1 in compare_scd_to_file \n");

  scd_count_t *current = head;
  while (current != NULL)
  {
    //  printf("Entering compare_scd_to_file - File 2 \n");
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
      printf("System call: %d, was not in the whitelist\n", current->syscall);
      all_matched = false;
    }
    current = current->next;
  }
  fclose(fp);

  // printf("Exiting compare_scd_to_file \n");
  return all_matched;
  // printf("Returning all_matched in compare_scd_to_file \n");
}

// function to check if all system calls matched
int check_all_matched(bool all_matched)
{
  if (all_matched)
  {
    //  printf("Entering IF statement check_all_matched \n");
    printf("All system calls matched\n");
    //  printf("Entering curl 1\n");
    //  printf("Exiting program check_all_matched \n");
    return 0;
  }
  else
  {
    // printf("Entering ELSE statement check_all_matched \n");
    printf("All system calls did not match\n");
    // printf("Entering curl 2\n");
    //  printf("Exiting program check_all_matched \n");
  }
  return 1;
}

// this function is called when the program exits to free the memory allocated for the linked list of system calls
void free_scd_list(mambo_context *ctx, scd_count_t *head)
{
  if (head == NULL)
  {
    return;
  }
  scd_count_t *last;
  last = head;
  head = head->next;
  mambo_free(ctx, last);

  while (head != NULL)
  {
    last = head;
    head = head->next;
    free(last);
  }
}

// function is called when a syscall is executed and adds it to the list
void scd_exe(scd_count_t *scd_list)
{
  assert(scd_list != NULL);
  inc_scd_syscall_in_list(scd_list, get_svc_type());
}

int scd_hook(mambo_context *ctx)
{
  if (ctx->code.inst == A64_SVC)
  {
    scd_count_t *scd_list = (scd_count_t *)mambo_get_thread_plugin_data(ctx);
    emit_push(ctx, (1 << reg0) | (1 << reg1) | (1 << reg2));
    emit_set_reg_ptr(ctx, reg0, scd_list);
    emit_mov(ctx, 1, 8);
    emit_safe_fcall(ctx, scd_exe, 1);
    emit_pop(ctx, (1 << reg0) | (1 << reg1) | (1 << reg2));
    return 0;
  }
}

// function is called when a thread is created and sets the plugin data to the list
int scd_pre_thread_handler(mambo_context *ctx)
{
  scd_count_t *scd_list = (scd_count_t *)mambo_alloc(ctx, sizeof(scd_count_t)); // allocate memory for the list
  assert(scd_list != NULL);                                                     // check if memory was allocated
  mambo_set_thread_plugin_data(ctx, scd_list);                                  // set the list as the thread's plugin data
}

// function is called when a thread is destroyed
int scd_post_thread_handler(mambo_context *ctx)
{
  scd_count_t *scd_list = (scd_count_t *)mambo_get_thread_plugin_data(ctx); // get the list from the thread's plugin data
  check_all_matched(compare_scd_to_file(scd_list, "whitelist.txt"));        // check if all system calls are in the whitelist
  write_scd_list_to_file("syscall_list_runtime.txt", scd_list);             // write the list to a file                 // write the list to a string file
  curl_post();                                                              // post the string file to the server
  print_scd_list(scd_list);                                                 // print the list
  free_scd_list(ctx, scd_list);                                             // free the memory allocated for the list
}

// function is called when the program exits
int scd_exit_handler(mambo_context *ctx)
{
  fprintf(stderr, "Exit called:\n"); // print exit message
}

// function is called when the plugin is loaded
__attribute__((constructor)) void scd_init_plugin() // __attribute__((constructor)) is a gcc extension
{
  mambo_context *ctx = mambo_register_plugin();                 // register the plugin
  assert(ctx != NULL);                                          // check if the plugin was registered
  mambo_register_pre_thread_cb(ctx, &scd_pre_thread_handler);   // register the pre thread handler
  mambo_register_pre_inst_cb(ctx, &scd_hook);                   // register the pre instruction handler
  mambo_register_post_thread_cb(ctx, &scd_post_thread_handler); // register the post thread handler
  mambo_register_exit_cb(ctx, &scd_exit_handler);               // register the exit handler
  setlocale(LC_NUMERIC, "");                                    // set the locale to the default
}

#endif
