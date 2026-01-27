#include "../Server/common.h"
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("ERROR: TOO FEW ARGUEMNTS");
        exit(1);
    }
    const char *file_name = argv[1];
    const char *task = argv[2];
    int fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (fd == -1)
    {
        perror("shm_open");
        exit(1);
    }
    struct data_t *data = mmap(NULL, sizeof(struct data_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED)
    {
        perror("mmap");
        exit(1);
    }
    int counter = 0;
    while (1)
    {
        if (sem_trywait(&data->sem_server) == 0)
        {
            break;
        }
        if (counter > 0)
        {
            printf("REQUEST TIMEOUT (5s)\n");
            exit(1);
        }
        printf("Server is running PLEASE WAIT\n");
        counter++;
        sleep(5);
    }
    if (strcmp(task, "min") == 0)
    {
        data->op = MIN;
    }
    else if (strcmp(task, "max") == 0)
    {
        data->op = MAX;
    }
    else
    {
        printf("ERROR: WRONG OPERATION\n");
        sem_post(&data->sem_server);
        exit(1);
    }
    FILE *fp = fopen(file_name, "r");
    if (!fp)
    {
        printf("ERROR: LOADING FILE\n");
        sem_post(&data->sem_server);
        exit(1);
    }
    int result = read_keyboard(fp, data);
    if (result != 0)
    {
        fclose(fp);
        sem_post(&data->sem_server);
        exit(1);
    }
    sem_post(&data->sem_request);
    sem_wait(&data->sem_respond);

    printf("Result: %d\n", data->result);

    sem_post(&data->sem_server);

    munmap(data, sizeof(struct data_t));
    close(fd);
    fclose(fp);
    return 0;
}
