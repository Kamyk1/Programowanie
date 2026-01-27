//
// Created by root on 1/14/26.
//
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <ctype.h>

#define SHM_NAME "/shm_open"
#define SEM_NAME "/sem_open"

pthread_mutex_t client_mutex = PTHREAD_MUTEX_INITIALIZER;

#ifndef SERVER_2026_01_14_COMMON_H
#define SERVER_2026_01_14_COMMON_H

enum operation_t
{
  MIN,
  MAX
};

struct data_t
{
  int32_t input[1000];

  size_t width;
  size_t height;

  enum operation_t op;
  int stat_max;
  int stat_min;
  int32_t result;

  sem_t sem_server;
  sem_t sem_request;
  sem_t sem_respond;

  int server_running;
};

void *handle_client(void *arg)
{
  struct data_t *data = (struct data_t *)arg;
  int min = data->input[0], max = data->input[0];
  for (size_t i = 0; i < data->height; i++)
  {
    for (size_t j = 0; j < data->width; j++)
    {
      if (data->op == MAX)
      {
        if (data->input[i * data->width + j] >= max)
        {
          max = data->input[i * data->width + j];
        }
      }
      else if (data->op == MIN)
      {
        if (data->input[i * data->width + j] <= min)
        {
          min = data->input[i * data->width + j];
        }
      }
    }
  }
  data->result = (data->op == MAX) ? max : min;
  if (data->op == MAX)
    data->stat_max++;
  else
    data->stat_min++;
  sem_post(&data->sem_respond);
  return NULL;
}

void *stats_timer_thread(void *arg)
{
  struct data_t *data = (struct data_t *)arg;
  while (data->server_running)
  {
    sleep(10);
    if (!data->server_running) break;
    printf("\n[AUTO-STAT] Total MIN: %d, Total MAX: %d\n", 
           data->stat_min, data->stat_max);
    printf("Enter command (quit/stat/reset): "); 
    fflush(stdout); 
  }
  return NULL;
}

void *command_thread(void *arg)
{
  struct data_t *data = (struct data_t *)arg;
  char buf[64];

  while (data->server_running)
  {
    printf("Enter command (quit/stat/reset): ");
    if (!fgets(buf, sizeof(buf), stdin))
      continue;
    buf[strcspn(buf, "\n")] = 0;

    if (strcmp(buf, "quit") == 0)
    {
      printf("Server shutting down...\n");
      data->server_running = 0;
    }
    else if (strcmp(buf, "stat") == 0)
    {
      printf("MIN: %d, MAX: %d\n", data->stat_min, data->stat_max);
    }
    else if (strcmp(buf, "reset") == 0)
    {
      data->stat_min = 0;
      data->stat_max = 0;
    }
    else
    {
      printf("Unknown command: %s\n", buf);
    }
  }
  return NULL;
}

int32_t read_keyboard(FILE *fp, struct data_t *data)
{
  int width = 0, height = 0;
  int curr_width = 0;
  int znak;
  int in_number = 0;

  while ((znak = fgetc(fp)) != EOF)
  {
    if (isdigit(znak) || znak == '-' || znak == '+')
    {
      if (!in_number)
      {
        curr_width++;
        in_number = 1;
      }
    }
    else if (isspace(znak))
    {
      in_number = 0;
      if (znak == '\n')
      {
        if (height == 0)
          width = curr_width;
        else if (curr_width != width)
        {
          printf("ERROR: LOADING FILE\n");
          return 1;
        }
        curr_width = 0;
        height++;
      }
    }
  }
  if (curr_width > 0)
  {
    if (height == 0)
      width = curr_width;
    else if (curr_width != width)
    {
      printf("ERROR: LOADING FILE\n");
      return 1;
    }
    height++;
  }
  
  rewind(fp);
  for (size_t i = 0; i < (size_t)height; i++)
  {
    for (size_t j = 0; j < (size_t)width; j++)
    {
      if (fscanf(fp, "%d", &data->input[i * width + j]) != 1)
      {
        printf("ERROR: READING DATA\n");
        return 1;
      }
    }
  }
  data->width = width;
  data->height = height;
  return 0;
}

#endif // SERVER_2026_01_14_COMMON_H
