//
// Created by root on 1/14/26.
//

#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#define SHM_NAME "/calc_shm"
#define SEM_NAME "/calc_sem"

#ifndef SERVER_2026_01_14_COMMON_H
#define SERVER_2026_01_14_COMMON_H

enum operator_t { ADD, SUB, DIV, MULT };

struct data_t {
  float a;
  float b;
  enum operator_t op;
  float result;
  int state;
};

#endif // SERVER_2026_01_14_COMMON_H
