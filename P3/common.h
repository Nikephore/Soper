#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <mqueue.h>
#include "pow.h"

#define SHM_NAME "/shm_memory"
#define MQ_NAME "/mq_example"
#define SEM_FILL "/sem_fill"
#define SEM_EMPTY "/sem_empty"
#define SEM_MUTEX "/sem_mutex"


#define BUFFER_SIZE 6
#define SHM_SIZE (BUFFER_SIZE * sizeof(Dato) + sizeof(int))
#define SHM_SIZE_STRUCT (BUFFER_SIZE * sizeof(Dato))

#define MAX_MSG 7
#define MAX_CYCLES 200
#define MIN_LAG 0
#define MAX_LAG 10000

typedef struct
{
    bool fin;
    long objetivo;
    long solucion;
    bool correcto;
} Dato;