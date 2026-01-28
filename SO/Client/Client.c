#include "../Server/common.h"

int32_t *read_fie(const char *file_name, size_t *count)
{
  FILE *fp = fopen(file_name, "rt");
  if (!fp)
  {
    printf("ERROR: FAILED TO OPEN FILE");
    return NULL;
  }
  int val;
  size_t temp_count = 0;
  while ((fscanf(fp, "%d", &val)) == 1)
  {
    temp_count++;
  }
  if (temp_count == 0)
  {
    printf("ERROR: FILE EMPTY");
    return NULL;
  }
  int32_t *result = calloc(temp_count, sizeof(int32_t));
  if (!result)
  {
    fclose(fp);
    printf("ERROR: FAILED TO ALLOCATE MEMORY");
    return NULL;
  }
  rewind(fp);
  temp_count = 0;
  while ((fscanf(fp, "%d", &val)) == 1)
  {
    result[temp_count] = val;
    temp_count++;
  }
  fclose(fp);
  *count = temp_count;
  return result;
}

int main(int argc, char *argv[])
{
  if (argc < 3)
  {
    printf("ERROR: NOT ENOUGH ARGUMENTS\n");
    return 1;
  }
  const char *file_name = argv[1];
  const char *task = argv[2];
  size_t count = 0;
  int32_t *input = read_fie(file_name, &count);
  if (!input)
  {
    return 1;
  }

  int control_fd = shm_open(SHM_CONTROL, O_RDWR, 0666);
  if (control_fd == -1)
  {
    free(input);
    error_handler(1);
  }
  struct control_t *control = mmap(NULL, sizeof(struct control_t), PROT_READ | PROT_WRITE, MAP_SHARED, control_fd, 0);
  if (control == MAP_FAILED)
  {
    free(input);
    error_handler(2);
  }
  int data_fd = shm_open(SHM_DATA, O_RDWR, 0666);
  if (data_fd == -1)
  {
    free(input);
    munmap(control, sizeof(struct control_t));
    error_handler(1);
  }
  if (ftruncate(data_fd, sizeof(int32_t) * count) == -1)
  {
    free(input);
    munmap(control, sizeof(struct control_t));
    error_handler(3);
  }
  int32_t *data = mmap(NULL, sizeof(int32_t) * count, PROT_READ | PROT_WRITE, MAP_SHARED, data_fd, 0);
  if (data == MAP_FAILED)
  {
    free(input);
    munmap(control, sizeof(struct control_t));
    error_handler(2);
  }
  if (sem_trywait(&control->sem_server) == -1)
  {
    free(input);
    munmap(data, sizeof(int32_t) * count);
    munmap(control, sizeof(struct control_t));
    error_handler(4);
  }

  memcpy(data, input, sizeof(int32_t) * count);
  control->count = count;
  control->op = (strcmp(task, "min") == 0) ? MIN : MAX;

  sem_post(&control->sem_request);
  sem_wait(&control->sem_respond);

  printf("%d", control->result);

  sem_post(&control->sem_server);

  munmap(control, sizeof(struct control_t));
  close(control_fd);
  close(data_fd);
  free(input);
  return 0;
}
