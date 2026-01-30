#include "common.h"

int32_t get_min(int32_t* tab, size_t count) {
  int32_t result = tab[0];
  for (size_t i = 0; i < count; i++) {
    if (tab[i] < result) {
      result = tab[i];
    }
  }
  return result;
}

int32_t get_max(int32_t* tab, size_t count) {
  int32_t result = tab[0];
  for (size_t i = 0; i < count; i++) {
    if (tab[i] > result) {
      result = tab[i];
    }
  }
  return result;
}

sem_t* sem_server_open;
int fd_data;

int max_count = 0;
int min_count = 0;

pthread_mutex_t mutex_stat = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t mutex_end = PTHREAD_MUTEX_INITIALIZER;

void* command_thread(void* arg) {
  struct control_t* control = (struct control_t*)arg;
  while (1) {
    printf("ENTER COMMAND: (quit/stat/reset)\n");
    char command[30];
    scanf("%30s", command);

    if (strcmp(command, "stat") == 0) {
      pthread_mutex_lock(&mutex_stat);
      printf("min: %d\nmax: %d\n", min_count, max_count);
      pthread_mutex_unlock(&mutex_stat);
    } else if (strcmp(command, "reset") == 0) {
      pthread_mutex_lock(&mutex_stat);
      max_count = min_count = 0;
      pthread_mutex_unlock(&mutex_stat);
    } else if (strcmp(command, "quit") == 0) {
      pthread_mutex_lock(&mutex_end);
      control->server_status = 0;
      pthread_mutex_unlock(&mutex_end);
      sem_post(&control->sem_request);
      break;
    } else {
      printf("WRONG COMMAND\n");
    }
  }
  return NULL;
}

void* processing_thread(void* arg) {
  struct control_t* control = (struct control_t*)arg;
  while (1) {
    sem_wait(&control->sem_request);

    pthread_mutex_lock(&mutex_end);
    if (control->server_status == 0) {
      pthread_mutex_unlock(&mutex_end);
      break;
    }
    pthread_mutex_unlock(&mutex_end);

    int32_t* data = mmap(NULL, control->size * sizeof(int32_t),
                         PROT_WRITE | PROT_READ, MAP_SHARED, fd_data, 0);
    if (data == MAP_FAILED) {
      break;
    }

    pthread_mutex_lock(&mutex_stat);
    control->result = (control->op == MIN) ? get_min(data, control->size)
                                           : get_max(data, control->size);
    pthread_mutex_unlock(&mutex_stat);
    pthread_mutex_lock(&mutex_stat);
    (control->op == MIN) ? min_count++ : max_count++;
    pthread_mutex_unlock(&mutex_stat);
    munmap(data, control->size * sizeof(int32_t));
    sem_post(&control->sem_respond);
  }
  return NULL;
}

int main() {
  sem_unlink(SEM_SERVER_FREE_NAME);
  sem_server_open = sem_open(SEM_SERVER_FREE_NAME, O_CREAT | O_EXCL, 0666, 0);

  int fd_control = shm_open(SHM_CONTROL, O_CREAT | O_RDWR, 0666);
  if (fd_control == -1) {
    sem_close(sem_server_open);
    sem_unlink(SEM_SERVER_FREE_NAME);
    printf("SHM_OPEN\n");
    return 1;
  }

  if (ftruncate(fd_control, sizeof(struct control_t)) == -1) {
    sem_close(sem_server_open);
    close(fd_control);
    return 1;
  }

  struct control_t* control =
      mmap(NULL, sizeof(struct control_t), PROT_WRITE | PROT_READ, MAP_SHARED,
           fd_control, 0);
  if (control == MAP_FAILED) {
    sem_close(sem_server_open);
    sem_unlink(SEM_SERVER_FREE_NAME);
    sem_unlink(SHM_CONTROL);
    printf("MMAP\n");
    return 1;
  }

  control->server_pid = getpid();
  printf("SERVER PID = %d\n", control->server_pid);

  fd_data = shm_open(SHM_DATA, O_CREAT | O_RDWR, 0666);
  if (fd_data == -1) {
    sem_close(sem_server_open);
    sem_unlink(SEM_SERVER_FREE_NAME);
    sem_unlink(SHM_CONTROL);
    munmap(control, sizeof(struct control_t));
    close(fd_control);
    return 1;
  }

  control->server_status = 1;
  sem_init(&control->sem_request, 1, 0);
  sem_init(&control->sem_respond, 1, 0);

  pthread_t command_id, processing_id;

  pthread_create(&command_id, NULL, command_thread, control);
  pthread_create(&processing_id, NULL, processing_thread, control);

  sem_post(sem_server_open);

  pthread_join(command_id, NULL);
  pthread_join(processing_id, NULL);

  sem_post(sem_server_open);

  sem_destroy(&control->sem_request);
  sem_destroy(&control->sem_respond);

  munmap(control, sizeof(struct control_t));
  close(fd_data);
  close(fd_control);
  sem_unlink(SEM_SERVER_FREE_NAME);
  shm_unlink(SHM_CONTROL);
  shm_unlink(SHM_DATA);
  return 0;
}
