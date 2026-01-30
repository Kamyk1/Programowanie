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

#define SHM_CONTROL "/shm_control"
#define SHM_DATA "/shm_data"

#ifndef SERVER_2026_01_14_COMMON_H
#define SERVER_2026_01_14_COMMON_H

enum operator_t { MIN, MAX };

struct control_t {
  pthread_mutex_t mutex;
  sem_t sem_server;
  sem_t sem_respond;
  sem_t sem_request;

  enum operator_t op;
  size_t size;
  int max_count;
  int min_count;
  int32_t result;
  pid_t server_id;
  int server_status;
};

void error_handler(size_t error_code) {
  switch (error_code) {
  case 1:
    perror("ERROR: SHM_OPEN\n");
    exit(1);
  case 2:
    perror("ERROR: FTRUNCATE\n");
    exit(2);
  case 3:
    perror("ERROR: MMAP\n");
    exit(3);
  case 4:
    printf("WRONG PID\n");
    exit(4);
  case 5:
    printf("SERVER IS NOT RUNNING\n");
    exit(5);
  case 6:
    perror("ERROR: SERVER IS BUSY\n");
    exit(6);
  }
}
#endif // SERVER_2026_01_14_COMMON_H
