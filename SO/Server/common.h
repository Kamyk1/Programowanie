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

#define NAME_SH "/open_sh"

#ifndef SERVER_2026_01_14_COMMON_H
#define SERVER_2026_01_14_COMMON_H

enum operator_t { MIN, MAX };

struct data_t {
  int32_t input[1000];
  int width;
  int height;

  enum operator_t op;
  sem_t sem_server;
  sem_t sem_respond;
  sem_t sem_request;

  int result;
  int min_count;
  int max_count;
  int server_status;
};

void *timer_response(void *arg) {
  struct data_t *data = (struct data_t *)arg;

  while (data->server_status) {
    if (!data->server_status)
      break;
    sleep(10);
    printf("MIN: %d, MAX: %d, TOTAL: %d\n", data->min_count, data->max_count,
           data->min_count + data->max_count);
    printf("Enter Command (quit/stat/reset)\n");
  }
  return NULL;
}

void *print_client(void *arg) {
  struct data_t *data = (struct data_t *)arg;
  char buff[64];
  while (data->server_status) {
    if (!data->server_status)
      break;
    printf("Enter Command (quit/stat/reset)\n");
    if (!fgets(buff, sizeof(buff), stdin)) {
      continue;
    }
    buff[strcspn(buff, "\n")] = 0;
    if (strcmp(buff, "quit") == 0) {
      printf("Server shutting down...\n");
      data->server_status = 0;
    } else if (strcmp(buff, "stat") == 0) {
      printf("MIN: %d, MAX: %d\n", data->min_count, data->max_count);
    } else if (strcmp(buff, "reset") == 0) {
      data->max_count = 0, data->min_count = 0;
    } else {
      printf("Wrong command\nsPlease try again\n");
    }
  }
  return NULL;
}

void *handle_client(void *arg) {
  struct data_t *data = (struct data_t *)arg;
  int min = data->input[0], max = data->input[0];
  int res = (data->op == MAX) ? 1 : 0;
  for (int i = 0; i < data->height; i++) {
    for (int j = 0; j < data->width; j++) {
      if (res == 1) {
        if (data->input[i * data->width + j] >= max) {
          max = data->input[i * data->width + j];
        }
      } else if (res == 0) {
        if (data->input[i * data->width + j] <= min) {
          min = data->input[i * data->width + j];
        }
      }
    }
  }
  data->result = (data->op == MAX) ? max : min;
  (data->op == MAX) ? data->max_count++ : data->min_count++;
  sem_post(&data->sem_respond);
  return NULL;
}

int read_keyboard(FILE *fp, struct data_t *data) {
  int width = 0, height = 0, curr_width = 0, in_number = 0;
  int c;
  while ((c = fgetc(fp)) != EOF) {
    if (isdigit(c) || c == '-' || c == '+') {
      if (!in_number) {
        curr_width++;
        in_number = 1;
      }
    } else if (isspace(c)) {
      in_number = 0;
      if (c == '\n') {
        if (height == 0) {
          width = curr_width;
        } else if (curr_width != width) {
          printf("ERROR: READING FILE\n");
          return 1;
        }
        curr_width = 0;
        height++;
      }
    }
  }
  rewind(fp);
  data->height = height;
  data->width = width;
  for (int i = 0; i < height; i++) {
    for (int j = 0; j < width; j++) {
      if (fscanf(fp, "%d", &data->input[i * width + j]) != 1) {
        printf("ERROR: LOADING FILE\n");
        return 1;
      }
    }
  }
  return 0;
}
#endif // SERVER_2026_01_14_COMMON_H
