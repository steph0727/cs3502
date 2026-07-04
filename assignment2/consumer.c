#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <fcntl.h>
#include "buffer.h"

int main(int argc, char *argv[]) {

int consumer_id = atoi(argv[1]);
int num_items = atoi(argv[2]);

int shm_id = shmget(SHM_KEY, sizeof(shared_buffer_t), IPC_CREAT |0666);
if (shm_id == -1){
perror ("shmget");
exit (EXIT_FAILURE);
}

 shared_buffer_t *shm = (shared_buffer_t *)shmat(shm_id, NULL, 0);
if (shm == (void *)-1) {
perror("shmat");
exit(EXIT_FAILURE);
}

sem_t *sem_empty = sem_open(SEM_EMPTY_NAME, O_CREAT, 0644, BUFFER_SIZE);
sem_t *sem_full  = sem_open(SEM_FULL_NAME,  O_CREAT, 0644, 0);
sem_t *sem_mutex = sem_open(SEM_MUTEX_NAME, O_CREAT, 0644, 1);

if (sem_empty == SEM_FAILED || sem_full == SEM_FAILED || sem_mutex == SEM_FAILED) {
perror("sem_open");
shmdt(shm);
exit(EXIT_FAILURE);
}
for (int i=0; i<num_items; i++){
sem_wait(sem_full);
sem_wait(sem_mutex);

item_t item = shm->buffer[shm->tail];
shm->tail = (shm->tail + 1) % BUFFER_SIZE;
shm->count--;

sem_post(sem_mutex);
sem_post(sem_empty);

printf("Consumer %d: Consumed value %d from Producer %d\n", consumer_id, item.value, item.producer_id);
fflush(stdout);
}

shmdt(shm);
sem_close(sem_empty);
sem_close(sem_full);
sem_close(sem_mutex);

return 0;
}
