#include "common.h"
#include <semaphore.h>
#include <stdio.h>

int main()
{
  int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
  if (fd == -1)
  {
    perror("shm_open");
    return 1;
  }

  ftruncate(fd, sizeof(struct data_t));

  struct data_t *data = mmap(NULL, sizeof(struct data_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

  sem_init(&data->server_sem, 1, 1);
  sem_init(&data->request_sem, 1, 0);
  sem_init(&data->response_sem, 1, 0);

  data->state = 0;

  printf("Serwer uruchomiony...\n");

  while (1)
  {
    sem_wait(&data->request_sem);
    if (data->state == 1)
    {
      switch (data->op)
      {
      case ADD:
        data->result = data->a + data->b;
        break;
      case SUB:
        data->result = data->a - data->b;
        break;
      case MULT:
        data->result = data->a * data->b;
        break;
      case DIV:
        data->result = (data->b != 0) ? data->a / data->b : 0;
        break;
      default:
        data->result = 0;
      }
      data->state = 2;
    }
    sem_post(&data->response_sem);
    sleep(1);
  }
  munmap(data, sizeof(struct data_t));
  close(fd);
  shm_unlink(SHM_NAME);
  return 0;
}
