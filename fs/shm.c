#include <sys/types.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void *connect(int *shmid, int proj_id, int size) {
    key_t key;
    void *shmadd;

    if ((key = ftok("./", proj_id)) == -1)
        perror("ftok error");

    if ((*shmid = shmget(key, size, IPC_CREAT | 0666)) < 0) {
        perror("shmget error");
        exit(-1);
    }
    // printf("connect shared-memory success, with shmid: %d\n", *shmid);

    if ((shmadd = shmat(*shmid, NULL, 0)) < 0) {
        perror("shmat error");
        exit(-1);
    }
    printf("%p\n", shmadd);
    return shmadd;
}
