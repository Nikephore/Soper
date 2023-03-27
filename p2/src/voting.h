#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <semaphore.h>
#include <fcntl.h>
#include <math.h>


#include "pow.h"

typedef struct
{
  pid_t voter_pid; /* pid del votante */
  int vote; /* Voto al proceso candidato 1=Y 0=N */
} voter;

void voter_process(struct sigaction sact, sem_t *sem_c, sem_t *sem_f, sem_t *sem_s, int n_procs);