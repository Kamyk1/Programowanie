#include "../Server/common.h"
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>

int main()
{
    pthread_t th;
    int fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (fd == -1)
    {
        perror("shm_open");
        return 1;
    }

    struct data_t *data = mmap(NULL, sizeof(struct data_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if(sem_trywait(&data->server_sem) == -1) {
        perror("sem_trywait");
        exit(1);
    }
    pthread_create(&th, NULL, input_keyboard, data);
    pthread_join(th, NULL);

    sem_post(&data->request_sem);
    sem_wait(&data->response_sem);

    printf("%f", data->result);
    data->state = 0;

    sem_post(&data->server_sem);
    munmap(data, sizeof(struct data_t));
    close(fd);
    return 0;
}
