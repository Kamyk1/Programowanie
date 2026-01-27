#include "common.h"
#include <semaphore.h>
#include <stdio.h>

int main()
{
  int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
  if (fd == -1)
  {
    perror("shm_open");
    exit(1);
  }
  ftruncate(fd, sizeof(struct data_t));
  struct data_t *data = mmap(NULL, sizeof(struct data_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (data == MAP_FAILED)
  {
    perror("mmap");
    exit(1);
  }
  memset(data, 0, sizeof(struct data_t));

  sem_init(&data->sem_server, 1, 1);
  sem_init(&data->sem_request, 1, 0);
  sem_init(&data->sem_respond, 1, 0);

  pthread_t th;
  data->server_running = 1;
  pthread_create(&th, NULL, command_thread, data);

  while (data->server_running)
  {
    if (sem_trywait(&data->sem_request) == 0)
    {
      pthread_t client_th;
      if (pthread_create(&client_th, NULL, handle_client, data) == 0)
      {
        pthread_detach(client_th);
      }
    }
    sleep(10);
  }

  pthread_join(th, NULL);

  munmap(data, sizeof(struct data_t));
  close(fd);
  shm_unlink(SHM_NAME);
  return 0;
}
