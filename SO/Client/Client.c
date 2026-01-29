#include "../Server/common.h"

int32_t *read_file(const char *file_name, size_t *size) {
  FILE *fp = fopen(file_name, "rt");
  if (!fp) {
    printf("ERROR: LOADING FILE");
    return NULL;
  }
  size_t count = 0;
  int val;
  while (fscanf(fp, "%d", &val) == 1) {
    count++;
  }
  if (count == 0) {
    fclose(fp);
    printf("ERROR: FILE EMPTY");
    return NULL;
  }
  int32_t *input = calloc(count, sizeof(int32_t));
  if (!input) {
    printf("ERROR: FAILED TO ALLOCATE MEMORY");
    fclose(fp);
    return NULL;
  }
  rewind(fp);
  count = 0;
  while (fscanf(fp, "%d", &val) == 1) {
    input[count] = val;
    count++;
  }
  *size = count;
  fclose(fp);
  return input;
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
    printf("ERROR: NOT ENOUGH ARGUMENTS\n");
    return 1;
  }
  const char *file_name = argv[1];
  const char *task = argv[2];
  size_t count = 0;
  int32_t *input = read_file(file_name, &count);
  if (!input) {
    return 1;
  }
  int fd_control = shm_open(SHM_CONTROL, O_RDWR, 0666);
  if (fd_control == -1) {
    free(input);
    error_handler(1);
  }
  struct control_t *control =
      mmap(NULL, sizeof(struct control_t), PROT_READ | PROT_WRITE, MAP_SHARED,
           fd_control, 0);
  if (control == MAP_FAILED) {
    free(input);
    error_handler(3);
  }

  printf("Server PID z SHM: %d\n", control->server_pid);

  if (kill(control->server_pid, 0) == -1)
    exit(1);

  int fd_data = shm_open(SHM_DATA, O_RDWR, 0666);
  if (fd_data == -1) {
    free(input);
    munmap(control, sizeof(struct control_t));
    error_handler(1);
  }
  if (ftruncate(fd_data, count * sizeof(int32_t)) == -1) {
    free(input);
    munmap(control, sizeof(struct control_t));
    error_handler(2);
  }
  int32_t *data = mmap(NULL, count * sizeof(int32_t), PROT_WRITE | PROT_READ,
                       MAP_SHARED, fd_data, 0);
  if (data == MAP_FAILED) {
    free(input);
    munmap(control, sizeof(struct control_t));
    error_handler(3);
  }
  if (sem_wait(&control->sem_server) == -1) {
    free(input);
    munmap(control, sizeof(struct control_t));
    munmap(data, count * sizeof(int32_t));
    error_handler(4);
  }
  control->op = (strcmp(task, "min") == 0) ? MIN : MAX;
  control->count = count;
  memcpy(data, input, count * sizeof(int32_t));

  sem_post(&control->sem_request);
  sem_wait(&control->sem_respond);

  printf("Result: %d\n", control->result);

  sem_post(&control->sem_server);

  munmap(control, sizeof(struct control_t));
  munmap(data, count * sizeof(int32_t));
  close(fd_control);
  close(fd_data);
  free(input);
  return 0;
}
