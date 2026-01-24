#include "common.h"
#include <semaphore.h>
#include <stdio.h>

int main() {
  sem_t *sem = sem_open(SEM_NAME, O_CREAT, 0666, 1);
  int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
  if (fd == -1) {
    perror("shm_open");
    exit(1);
  }

  ftruncate(fd, sizeof(struct data_t));

  struct data_t *data = mmap(NULL, sizeof(struct data_t),
                             PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

  data->state = 0;

  printf("Serwer uruchomiony...\n");

  while (1) {
    sleep(10);
    if (data->state == 1) {
      switch (data->op) {
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
  }

  munmap(data, sizeof(struct data_t));
  close(fd);
  shm_unlink(SHM_NAME);
  sem_close(sem);
  return 0;
}
