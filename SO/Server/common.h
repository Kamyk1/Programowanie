//
// Created by root on 1/14/26.
//
#include <ctype.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#define SHM_DATA "/shm_data"
#define SHM_CONTROL "/shm_control"

#ifndef SERVER_2026_01_14_COMMON_H
#define SERVER_2026_01_14_COMMON_H

enum operator_t
{
  MIN,
  MAX
};

struct control_t
{
  enum operator_t op;
  size_t count;
  int32_t result;
  size_t count_min;
  size_t count_max;

  sem_t sem_server;
  sem_t sem_request;
  sem_t sem_respond;
  pthread_mutex_t mutex;

  int server_status;
};

void error_handler(size_t error_code)
{
  switch (error_code)
  {
  case 1:
    perror("shm_open");
    exit(1);
  case 2:
    perror("failed to map");
    exit(2);
  case 3:
    perror("failed to truncate");
    exit(3);
  case 4:
    printf("Server is busy");
    exit(4);
  }
}

#endif // SERVER_2026_01_14_COMMON_H
