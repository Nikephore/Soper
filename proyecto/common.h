/**
 * @file common.h
 * @author Luis Miguel Nucifora & Alexis Canales Molina.
 * @brief Common functions and constants used by both miner and monitor
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <math.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <mqueue.h>
#include "pow.h"

#define SHM_MONITOR "/shm_monitor"
#define SHM_MINERO "/shm_minero"
#define MQ_NAME "/mq_example"

#define SEM_FILL "/sem_fill"
#define SEM_EMPTY "/sem_empty"
#define SEM_MUTEX "/sem_mutex"

#define BUFFER_SIZE 5
#define SHM_SIZE_MINERO (sizeof(MemoriaMinero))
#define SHM_SIZE_MONITOR (sizeof(MemoriaMonitor))

#define MAX_MSG 10
#define MAX_CYCLES 200
#define MAX_MINEROS 50
#define MAX_FILE_SIZE 20

#define FOUND 0
#define NOT_FOUND -1

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
    Cartera carteras[MAX_MINEROS];
    unsigned int votos_totales;
    unsigned int votos_positivos;
    bool correcto;
    bool fin;
} Bloque;


/**
 * Estructura de datos que se guardarán
 * en la memoria compartida de los mineros.
*/
typedef struct
{
    Cartera carteras[MAX_MINEROS];
    bool votos[MAX_MINEROS];
    Bloque bl_ultimo;
    Bloque bl_actual;
    int num_mineros;

} MemoriaMinero;

/**
 * Estructura de datos que se guardarán
 * en la memoria compartida entre monitor
 * y comprobador.
*/
typedef struct
{
    Bloque bloque[BUFFER_SIZE];
    int front;
    int rear;

} MemoriaMonitor;

/**
 * Estructura de datos con el rango de
 * búsqueda de cada hilo, el objetivo
 * y la solución
*/
typedef struct
{
  int min;
  int max;
  long objetivo;
  long solucion;
} Busqueda;


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
