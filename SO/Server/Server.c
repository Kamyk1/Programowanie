#include "common.h"

int fd_data;

int32_t get_min(int32_t *tab, size_t size) {
  int32_t min = tab[0];
  for (size_t i = 0; i < size; i++) {
    if (tab[i] < min) {
      min = tab[i];
    }
  }
  return min;
}

int32_t get_max(int32_t *tab, size_t size) {
  int32_t max = tab[0];
  for (size_t i = 0; i < size; i++) {
    if (tab[i] > max) {
      max = tab[i];
    }
  }
  return max;
}

void *command_print(void *arg) {
  struct control_t *control = (struct control_t *)arg;
  while (1) {
    char buffer[30] = {0};
    scanf("%29s", buffer);
    while (getchar() != '\n')
      ;
    if (!strcmp(buffer, "stat")) {
      pthread_mutex_lock(&control->mutex);
      printf("MIN: %d, MAX: %d\n", control->min_count, control->max_count);
      pthread_mutex_unlock(&control->mutex);
    } else if (!strcmp(buffer, "reset")) {
      pthread_mutex_lock(&control->mutex);
      control->min_count = 0;
      control->max_count = 0;
      pthread_mutex_unlock(&control->mutex);
    } else if (!strcmp(buffer, "quit")) {
      pthread_mutex_lock(&control->mutex);
      control->server_status = 0;
      pthread_mutex_unlock(&control->mutex);
      sem_post(&control->sem_request);
      break;
    } else {
      printf("wrong command\n");
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
    int32_t *shared_tab = mmap(NULL, sizeof(int32_t) * control->count,
                               PROT_WRITE | PROT_READ, MAP_SHARED, fd_data, 0);
    if (shared_tab == MAP_FAILED) {
      break;
    }
    pthread_mutex_unlock(&control->mutex);
    pthread_mutex_lock(&control->mutex);
    control->result = (control->op == MIN)
                          ? get_min(shared_tab, control->count)
                          : get_max(shared_tab, control->count);

    (control->op == MIN) ? control->min_count++ : control->max_count++;
    pthread_mutex_unlock(&control->mutex);
    munmap(shared_tab, sizeof(int32_t) * control->count);
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
    error_handler(2);
  }
  struct control_t *control =
      mmap(NULL, sizeof(struct control_t), PROT_READ | PROT_WRITE, MAP_SHARED,
           fd_control, 0);
  if (control == MAP_FAILED) {
    error_handler(3);
  }

  control->server_pid = getpid();
  printf("Server started with PID: %d\n", control->server_pid);

  fd_data = shm_open(SHM_DATA, O_CREAT | O_RDWR, 0666);
  if (fd_data == -1) {
    munmap(control, sizeof(struct control_t));
    error_handler(1);
  }

  control->result = 0;
  control->max_count = 0;
  control->min_count = 0;
  control->server_status = 1;

  sem_init(&control->sem_server, 1, 1);
  sem_init(&control->sem_respond, 1, 0);
  sem_init(&control->sem_request, 1, 0);

  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
  pthread_mutex_init(&control->mutex, &attr);

  pthread_t client_th, command_th;

  pthread_create(&client_th, NULL, client_handler, control);
  pthread_create(&command_th, NULL, command_print, control);

  pthread_join(command_th, NULL);
  sem_post(&control->sem_request);
  pthread_join(client_th, NULL);

  pthread_mutex_destroy(&control->mutex);

  sem_destroy(&control->sem_request);
  sem_destroy(&control->sem_respond);
  sem_destroy(&control->sem_server);

  munmap(control, sizeof(struct control_t));
  close(fd_control);
  shm_unlink(SHM_CONTROL);
  shm_unlink(SHM_DATA);
  printf("laza");
  return 0;
}
