#include "common.h"
#include <stdio.h>

int main() {
  int fd = shm_open(NAME_SH, O_CREAT | O_RDWR, 0666);
  if (fd == -1) {
    perror("shm_open");
    exit(1);
  }
  ftruncate(fd, sizeof(struct data_t));
  struct data_t *data = mmap(NULL, sizeof(struct data_t),
                             PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (data == MAP_FAILED) {
    perror("mmap");
    exit(1);
  }

  data->server_status = 1;
  data->max_count = 0;
  data->min_count = 0;

  sem_init(&data->sem_server, 1, 1);
  sem_init(&data->sem_request, 1, 0);
  sem_init(&data->sem_respond, 1, 0);

  pthread_t response_th;
  pthread_create(&response_th, NULL, timer_response, data);
  pthread_t print_th;
  pthread_create(&print_th, NULL, print_client, data);
  while (data->server_status) {
    if (sem_trywait(&data->sem_request) == 0) {
      pthread_t client_th;
      pthread_create(&client_th, NULL, handle_client, data);
      pthread_detach(client_th);
    }
    usleep(10000);
  }
  pthread_join(response_th, NULL);
  pthread_join(print_th, NULL);
  munmap(data, sizeof(struct data_t));
  close(fd);
  shm_unlink(NAME_SH);
  return 0;
}
