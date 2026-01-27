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
#include <stdio.h>

#define SHM_NAME "/shm_open"
#define SEM_NAME "/sem_open"
pthread_mutex_t mt_inner = PTHREAD_MUTEX_INITIALIZER;

#ifndef SERVER_2026_01_14_COMMON_H
#define SERVER_2026_01_14_COMMON_H

enum operator_t
{
  ADD,
  DIV,
  SUB,
  MULT
};

struct data_t
{
  float a;
  float b;
  enum operator_t op;
  float result;
  int state;
  sem_t server_sem;
  sem_t request_sem;
  sem_t response_sem;
};

void *input_keyboard(void *arg)
{
  struct data_t *data = (struct data_t *)arg;
  enum operator_t op;
  float a, b;
  char c;
  printf("Podaj równanie: (a + b)");
  if (scanf("%f %c %f", &a, &c, &b) != 3)
  {
    printf("Invalid Input\n");
    return NULL;
  }
  switch (c)
  {
  case '+':
    op = ADD;
    break;
  case '-':
    op = SUB;
    break;
  case '*':
    op = MULT;
    break;
  case '/':
    op = DIV;
    break;
  default:
    printf("Wrong operation\n");
    return NULL;
  }
  pthread_mutex_lock(&mt_inner);
  data->a = a;
  data->b = b;
  data->op = op;
  data->state = 1;
  pthread_mutex_unlock(&mt_inner);
  return NULL;
}

#endif // SERVER_2026_01_14_COMMON_H
