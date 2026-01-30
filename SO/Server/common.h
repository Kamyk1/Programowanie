//
// Created by root on 1/14/26.
//
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#define SEM_SERVER_FREE_NAME "/server_free"
#define SHM_CONTROL "/shm_server"
#define SHM_DATA "/shm_data"

#ifndef SERVER_2026_01_14_COMMON_H
#define SERVER_2026_01_14_COMMON_H

enum operator_t { MIN, MAX };

struct control_t {
  enum operator_t op;
  int32_t result;
  size_t size;

  sem_t sem_respond;
  sem_t sem_request;

  pid_t server_pid;

  int server_status;
};

#endif  // SERVER_2026_01_14_COMMON_H
