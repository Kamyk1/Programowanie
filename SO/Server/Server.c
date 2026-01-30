#include "common.h"
#include <bits/pthreadtypes.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/mman.h>

int32_t get_min(int32_t *tab, size_t count) {
  int32_t result = tab[0];
  for (size_t i = 0; i < count; i++) {
    if (tab[i] < result) {
      result = tab[i];
    }
  }
  return result;
}

int32_t get_max(int32_t *tab, size_t count) {
  int32_t result = tab[0];
  for (size_t i = 0; i < count; i++) {
    if (tab[i] > result) {
      result = tab[i];
    }
  }
  return result;
}

int fd_data;

void *command_handler(void *arg) {
  struct control_t *control = (struct control_t *)arg;
  while (1) {
    printf("ENTER COMMAND: (quit/stat/reset)\n");
    char buff[32] = {0};
    scanf("%31s", buff);
    if (strcmp(buff, "stat") == 0) {
      pthread_mutex_lock(&control->mutex);
      printf("MIN: %d, MAX: %d\n", control->min_count, control->max_count);
      pthread_mutex_unlock(&control->mutex);
    } else if (strcmp(buff, "reset") == 0) {
      pthread_mutex_lock(&control->mutex);
      control->min_count = 0, control->max_count = 0;
      pthread_mutex_unlock(&control->mutex);
    } else if (strcmp(buff, "quit") == 0) {
      pthread_mutex_lock(&control->mutex);
      control->server_status = 0;
      pthread_mutex_unlock(&control->mutex);
      sem_post(&control->sem_request);
      break;
    } else {
      printf("WRONG COMMAND\n");
    }
  }
  return NULL;
}
void *client_handler(void *arg) {
  struct control_t *control = (struct control_t *)arg;
  while (1) {
    sem_wait(&control->sem_request);
    pthread_mutex_lock(&control->mutex);
    if (control->server_status == 0) {
      pthread_mutex_unlock(&control->mutex);
      break;
    }
    pthread_mutex_unlock(&control->mutex);
    pthread_mutex_lock(&control->mutex);
    int32_t *data = mmap(NULL, control->size * sizeof(int32_t),
                         PROT_WRITE | PROT_READ, MAP_SHARED, fd_data, 0);
    if (data == MAP_FAILED) {
      pthread_mutex_unlock(&control->mutex);
      break;
    }
    pthread_mutex_unlock(&control->mutex);
    pthread_mutex_lock(&control->mutex);
    control->result = (control->op == MIN) ? get_min(data, control->size)
                                           : get_max(data, control->size);
    pthread_mutex_unlock(&control->mutex);
    pthread_mutex_lock(&control->mutex);
    (control->op == MIN) ? control->min_count++ : control->max_count++;
    pthread_mutex_unlock(&control->mutex);
    munmap(data, control->size * sizeof(int32_t));
    sem_post(&control->sem_respond);
  }
  return NULL;
}

int main() {
  int fd_control = shm_open(SHM_CONTROL, O_CREAT | O_RDWR, 0666);
  if (fd_control == -1) {
    error_handler(1);
  }
  if (ftruncate(fd_control, sizeof(struct control_t)) == -1) {
    close(fd_control);
    shm_unlink(SHM_CONTROL);
    error_handler(2);
  }
  struct control_t *control =
      mmap(NULL, sizeof(struct control_t), PROT_READ | PROT_WRITE, MAP_SHARED,
           fd_control, 0);
  if (control == MAP_FAILED) {
    close(fd_control);
    shm_unlink(SHM_CONTROL);
    error_handler(3);
  }
  memset(control, 0, sizeof(struct control_t));
  printf("SERVER IS RUNNING...\n");
  control->server_id = getpid();
  printf("SERVER PID IS: %d\n", control->server_id);

  fd_data = shm_open(SHM_DATA, O_CREAT | O_RDWR, 0666);
  if (fd_data == -1) {
    close(fd_control);
    munmap(control, sizeof(struct control_t));
    error_handler(1);
  }

  control->server_status = 1;

  sem_init(&control->sem_server, 1, 1);
  sem_init(&control->sem_request, 1, 0);
  sem_init(&control->sem_respond, 1, 0);

  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
 pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_DEFAULT); 
pthread_mutex_init(&control->mutex, &attr);
pthread_mutexattr_destroy(&attr);

  pthread_t client_th, command_th;

  pthread_create(&command_th, NULL, command_handler, (void *)control);
  pthread_create(&client_th, NULL, client_handler, (void *)control);

  pthread_join(command_th, NULL);
  pthread_join(client_th, NULL);
  // sem_post(&control->sem_request);
  usleep(10000);
  pthread_mutex_destroy(&control->mutex);
  pthread_mutexattr_destroy(&attr);
  sem_destroy(&control->sem_request);
  sem_destroy(&control->sem_respond);
  sem_destroy(&control->sem_server);

  close(fd_control);
  close(fd_data);
  munmap(control, sizeof(struct control_t));
  shm_unlink(SHM_CONTROL);
  shm_unlink(SHM_DATA);
  return 0;
}
