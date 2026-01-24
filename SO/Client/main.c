#include "../Server/common.h"
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <sys/mman.h>

int main() {
  sem_t *sem = sem_open(SEM_NAME, 0);
  if (sem == SEM_FAILED) {
    perror("sem_open");
    exit(1);
  }
  sem_wait(sem);
  int fd = shm_open(SHM_NAME, O_RDWR, 0666);

  struct data_t *data = mmap(NULL, sizeof(struct data_t),
                             PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  data->op = ADD;
  data->a = 5;
  data->b = 3;
  data->state = 1;

  while (data->state != 2) {
    sleep(10);
  }

  printf("Wynik: %f\n", data->result);

  data->state = 0;
  sem_post(sem);
  sem_close(sem);
  munmap(data, sizeof(struct data_t));
  close(fd);
  return 0;
}
