#include "../Server/common.h"

int32_t* read_file(const char* file_name, size_t* count) {
  FILE* fp = fopen(file_name, "rt");
  if (!fp) {
    printf("ERROR: LOADING FILE\n");
    return NULL;
  }
  int temp_size = 0;
  int val;
  while (fscanf(fp, "%d", &val) == 1) {
    temp_size++;
  }
  int32_t* result = calloc(temp_size, sizeof(int32_t));
  if (!result) {
    fclose(fp);
    printf("ERROR: FAILED TO ALLOCATE MEMORY\n");
    return NULL;
  }
  rewind(fp);
  temp_size = 0;
  while (fscanf(fp, "%d", &val) == 1) {
    result[temp_size] = val;
    temp_size++;
  }
  *count = temp_size;
  fclose(fp);
  return result;
}

int main(int argc, char* argv[]) {
  if (argc < 3) {
    printf("NOT ENOUGH ARGUMENTS\n");
    return 1;
  }
  const char* file_name = argv[1];
  const char* task = argv[2];
  if (strcmp(task, "min") == 0 && strcmp(task, "max") == 0) {
    printf("WRONG TASK\n");
    return 1;
  }
  sem_t* sem_server_open = sem_open(SEM_SERVER_FREE_NAME, 0);
  if (sem_server_open == NULL) {
    sem_close(sem_server_open);
    printf("SERVER IS NOT RUNNING\n");
    return 0;
  }
  if (sem_trywait(sem_server_open) == -1) {
    sem_close(sem_server_open);
    printf("Server is busy\n");
    return 0;
  }

  int fd_control = shm_open(SHM_CONTROL, O_RDWR, 0666);
  if (fd_control == -1) {
    sem_close(sem_server_open);
    printf("SHM_OPEN\n");
    return 1;
  }
  struct control_t* control =
      mmap(NULL, sizeof(struct control_t), PROT_WRITE | PROT_READ, MAP_SHARED,
           fd_control, 0);
  if (control == MAP_FAILED) {
    sem_close(sem_server_open);
    printf("MMAP\n");
    return 1;
  }
  if (kill(control->server_pid, 0) == -1) {
    sem_close(sem_server_open);
    munmap(control, sizeof(struct control_t));
    close(fd_control);
    printf("WRONG PID\n");
    return 1;
  }

  printf("SERVER PID = %d\n", control->server_pid);

  int fd_data = shm_open(SHM_DATA, O_RDWR, 0666);
  if (fd_data == -1) {
    sem_close(sem_server_open);
    munmap(control, sizeof(struct control_t));
    close(fd_control);
    return 1;
  }
  size_t size = 0;
  int32_t* result = read_file(file_name, &size);
  if (ftruncate(fd_data, size * sizeof(int32_t)) == -1) {
    sem_close(sem_server_open);
    munmap(control, sizeof(struct control_t));
    close(fd_control);
    close(fd_data);
    return 1;
  }
  int32_t* data = mmap(NULL, size * sizeof(int32_t), PROT_WRITE | PROT_READ,
                       MAP_SHARED, fd_data, 0);
  if (data == MAP_FAILED) {
    sem_close(sem_server_open);
    free(result);
    munmap(control, sizeof(struct control_t));
    close(fd_control);
    close(fd_data);
    return 1;
  }
  memcpy(data, result, size * sizeof(int32_t));
  free(result);
  control->size = size;
  control->op = (strcmp(task, "min") == 0) ? MIN : MAX;

  sem_post(&control->sem_request);
  sem_wait(&control->sem_respond);

  printf("RESULT: %d\n", control->result);

  sem_post(sem_server_open);
  sem_close(sem_server_open);
  munmap(control, sizeof(struct control_t));
  munmap(data, size * sizeof(int32_t));
  close(fd_control);
  close(fd_data);
  return 0;
}
