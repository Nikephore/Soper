#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

#define SHM_MONITOR "/shm_monitor"
#define SHM_MINERO "/shm_minero"
#define MQ_NAME "/mq_example"


#define SEM_MUTEX "/sem_mutex"

#define BUFFER_SIZE 5
#define SHM_SIZE_MINERO (sizeof(MemoriaMinero))
#define SHM_SIZE_MONITOR (sizeof(MemoriaMonitor))

#define MAX_MSG 7
#define MAX_CYCLES 200
#define MAX_MINERS 1

/**
 * estructura de la cartera de un minero. 
 * Contiene su PID y sus monedas obtenidas.
*/
typedef struct
{
    pid_t id_proceso;
    unsigned int monedas;
} Cartera;

/**
 * Estructura de un bloque de minado.
*/
typedef struct
{
    int id;
    long objetivo;
    long solucion;
    pid_t ganador;
    Cartera *carteras;
    unsigned int votos_totales;
    unsigned int votos_positivos;
    bool correcto;
    bool fin;
} Bloque;


/**
 * Estructura de datos que se guardarán
 * en la memoria compartida.
*/
typedef struct
{
    Cartera *carteras;
    bool *votos;
    Bloque bl_ultimo;
    Bloque bl_actual;

} MemoriaMinero;

typedef struct
{
    Bloque bloque[BUFFER_SIZE];
    int front;
    int rear;

} MemoriaMonitor;




/**
 * @brief Check if the argument given is in a specific and valid range.
 *
 * @param min   Minimum number posible of the value.
 * @param max   Maximum number posible of the value.
 * @param value Value to check.
 * @param msg   Message to print with an identificative name of the variable.
 */
void number_range_error_handler(int min, int max, int value, char *msg);


/**
 * @brief Check if the argument given is a correct pid.
 *
 * @param pid   Process id to check.
 */
void pid_error_handler(pid_t pid);
