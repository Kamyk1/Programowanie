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
#include <unistd.h>

#define SHM_CONTROL "/shm_control"
#define SHM_DATA "/shm_data"

#ifndef SERVER_2026_01_14_COMMON_H
#define SERVER_2026_01_14_COMMON_H

enum operator_t { MIN, MAX };

struct control_t {
  pid_t server_pid;

  enum operator_t op;
  size_t count;
  int max_count;
  int min_count;
  int32_t result;

  sem_t sem_server;
  sem_t sem_respond;
  sem_t sem_request;

  pthread_mutex_t mutex;

  int server_status;
};

void error_handler(size_t error_code) {
  switch (error_code) {
  case 1:
    perror("ERROR: FAILED TO SHM_OPEN");
    exit(1);
  case 2:
    perror("ERROR: FAILED TO TRUNCATE");
    exit(2);
  case 3:
    perror("ERROR: FAILED TO MMAP");
    exit(3);
  case 4:
    printf("");
    exit(4);
  }
}
#endif // SERVER_2026_01_14_COMMON_H
