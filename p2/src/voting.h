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

typedef struct
{
  pid_t voter_pid; /* pid del votante */
  int vote; /* Voto al proceso candidato 1=Y 0=N */
} voter;