#include "../Server/common.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
  if (argc < 3) {
    printf("TOO LITTLE ARGUEMNTS\n");
    exit(1);
  }
  printf("Kamil Krukowicz Ocena: 4\n");
  const char *file_name = argv[1];
  const char *task = argv[2];
  int fd = shm_open(NAME_SH, O_RDWR, 0666);
  if (fd == -1) {
    perror("shm_open");
    exit(1);
  }
  struct data_t *data = mmap(NULL, sizeof(struct data_t),
                             PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (data == MAP_FAILED) {
    perror("mmap");
    exit(1);
  }
  int count = 0;
  while (1) {
    if (count > 0) {
      printf("REQUEST TIMEOUT\n");
      exit(1);
    }
    if (sem_trywait(&data->sem_server) == 0) {
      break;
    }
    printf("Server is running\n");
    sleep(5);
    count++;
  }
  FILE *fp = fopen(file_name, "r");
  if (!fp) {
    printf("ERROR: FILE DOESNT EXITS");
    sem_post(&data->sem_server);
    exit(1);
  }
  int error = read_keyboard(fp, data);
  if (error != 0) {
    fclose(fp);
    sem_post(&data->sem_server);
    return 1;
  }
  data->op = (strcmp(task, "min") == 0 ? MIN : MAX);
  sem_post(&data->sem_request);
  sem_wait(&data->sem_respond);
  printf("%d", data->result);
  sem_post(&data->sem_server);
  munmap(data, sizeof(struct data_t));
  fclose(fp);
  close(fd);
  return 0;
}
