#include "../Server/common.h"
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>

int main() {
  int fd = shm_open(SHM_NAME, O_RDWR, 0666);

  struct data_t *data = mmap(NULL, sizeof(struct data_t),
                             PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

  sem_wait(&data->sem);
  data->op = ADD;
  data->a = 5;
  data->b = 3;
  data->state = 1;

  sem_post(&data->sem);
  while (data->state != 2) {
    sleep(1);
  }

  printf("Wynik: %f\n", data->result);

  data->state = 0;
  sem_post(&data->sem);
  munmap(data, sizeof(struct data_t));
  close(fd);
  return 0;
}
