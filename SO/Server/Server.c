#include "common.h"

int data_fd;

int32_t get_max(int32_t *tab, size_t size)
{
  int32_t res = tab[0];
  for (size_t i = 0; i < size; ++i)
  {
    if (tab[i] > res)
      res = tab[i];
  }
  return res;
}

int32_t get_min(int32_t *tab, size_t size)
{
  int32_t res = tab[0];
  for (size_t i = 0; i < size; ++i)
  {
    if (tab[i] < res)
      res = tab[i];
  }
  return res;
}

void *operate_client(void *arg)
{
  struct control_t *control = (struct control_t *)arg;
  while (1)
  {
    sem_wait(&control->sem_request);
    pthread_mutex_lock(&control->mutex);
    if (control->server_status == 0)
    {
      pthread_mutex_unlock(&control->mutex);
      break;
    }
    pthread_mutex_unlock(&control->mutex);
    pthread_mutex_lock(&control->mutex);
    int32_t *shared_tab = mmap(NULL, sizeof(int32_t) * control->count, PROT_WRITE | PROT_READ, MAP_SHARED, data_fd, 0);
    if (shared_tab == MAP_FAILED)
    {
      break;
    }
    pthread_mutex_unlock(&control->mutex);
    pthread_mutex_lock(&control->mutex);
    control->result = (control->op == MIN) ? get_min(shared_tab, control->count) : get_max(shared_tab, control->count);

    (control->op == MIN) ? control->count_min++ : control->count_max++;
    pthread_mutex_unlock(&control->mutex);
    munmap(shared_tab, sizeof(int32_t) * control->count);
    sem_post(&control->sem_respond);
  }
  return NULL;
}

void *command_handler(void *arg)
{
  struct control_t *control = (struct control_t *)arg;
  while (1)
  {
    char buffer[30] = {0};
    scanf("%29s", buffer);
    while (getchar() != '\n')
      ;
    if (!strcmp(buffer, "stat"))
    {
      pthread_mutex_lock(&control->mutex);
      printf("MIN: %d, MAX: %d\n", control->count_min, control->count_max);
      pthread_mutex_unlock(&control->mutex);
    }
    else if (!strcmp(buffer, "reset"))
    {
      pthread_mutex_lock(&control->mutex);
      control->count_min = 0;
      control->count_max = 0;
      pthread_mutex_unlock(&control->mutex);
    }
    else if (!strcmp(buffer, "quit"))
    {
      pthread_mutex_lock(&control->mutex);
      control->server_status = 0;
      pthread_mutex_unlock(&control->mutex);
      sem_post(&control->sem_request);
      sem_post(&control->sem_respond);
      break;
    }
    else
    {
      printf("wrong command\n");
    }
  }

  return NULL;
}

int main()
{
  int control_fd = shm_open(SHM_CONTROL, O_CREAT | O_RDWR, 0666);
  if (control_fd == -1)
  {
    error_handler(1);
  }
  if (ftruncate(control_fd, sizeof(struct control_t)) == -1)
  {
    error_handler(3);
  }
  struct control_t *control = mmap(NULL, sizeof(struct control_t), PROT_READ | PROT_WRITE, MAP_SHARED, control_fd, 0);
  if (control == MAP_FAILED)
  {
    error_handler(2);
  }
  data_fd = shm_open(SHM_DATA, O_CREAT | O_RDWR, 0666);
  if (data_fd == -1)
  {
    munmap(control, sizeof(struct control_t));
    error_handler(1);
  }
  printf("Server is running...\n");
  memset(control, 0, sizeof(struct control_t));
  control->result = 0;
  control->count_max = 0;
  control->count_min = 0;
  control->server_status = 1;
  sem_init(&control->sem_server, 1, 1);
  sem_init(&control->sem_request, 1, 0);
  sem_init(&control->sem_respond, 1, 0);

  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
  pthread_mutex_init(&control->mutex, &attr);

  pthread_t client_th, command_th;
  pthread_create(&client_th, NULL, operate_client, control);
  pthread_create(&command_th, NULL, command_handler, control);

  pthread_join(client_th, NULL);
  pthread_join(command_th, NULL);

  pthread_mutex_destroy(&control->mutex);

  sem_destroy(&control->sem_request);
  sem_destroy(&control->sem_respond);
  sem_destroy(&control->sem_server);

  munmap(control, sizeof(struct control_t));
  close(control_fd);
  shm_unlink(SHM_CONTROL);
  return 0;
}
