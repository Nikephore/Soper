#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#define SHM_NAME "/shm_memory"

typedef struct {
    bool fin;
    long solucion;
    bool correcto;
} Dato;

void monitor(int memory, int time){
    Dato * mapped;
    if ((mapped = mmap(NULL, 6*sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, memory, 0)) == MAP_FAILED) {
        perror("mmap");
        close(memory);
        exit(EXIT_FAILURE);
    }
    close(memory);

    sem_wait(sem_fill);
    sem_wait(sem_mutex);
    extraerElemento();
    sem_post(sem_mutex);
    sem_post(sem_empty);
}

void comprobador(int memory, int time){
    Dato* mapped;
    if ((mapped = mmap(NULL, 6*sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, memory, 0)) == MAP_FAILED) {
        perror("mmap");
        close(memory);
        exit(EXIT_FAILURE);
    }
    close(memory);
    
    while(1){
        if(/*Comprobar si el booleano fin de dato es true*/0){
            sem_wait(sem_empty);
            sem_wait(sem_mutex);
            anadirElemento();
            sem_post(sem_mutex);
            sem_post(sem_fill);
            exit(EXIT_SUCCESS);
        }
        sem_wait(sem_empty);
        sem_wait(sem_mutex);
        anadirElemento();
        sem_post(sem_mutex);
        sem_post(sem_fill);
    }
}

int main(int argc, char** argv){
    int memory=0;

    memory = shm_open(SHM_NAME, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
    if (memory == -1) {
        if (errno == EEXIST) {
            memory = shm_open(SHM_NAME, O_RDWR, 0);
            if (memory == -1) {
                perror("Error opening the shared memory segment");
                exit(EXIT_FAILURE);
            } else {
                printf("Shared memory segment open\n");
                monitor(memory, atoi(argv[1]));
            }
        } else {
            perror("Error creating the shared memory segment\n");
            exit(EXIT_FAILURE);
        }
    } else {
        printf("Shared memory segment created\n");
        if (ftruncate(memory, 6*sizeof(Dato)) == -1) {
            perror("ftruncate");
            close(memory);
            exit(EXIT_FAILURE);
        }
        comprobador(memory, atoi(argv[1]));
    }

    return 0;
}