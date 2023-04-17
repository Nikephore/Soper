#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <mqueue.h>
#include "pow.h"

#define SHM_NAME "/shm_memory"
#define QUEUE_NAME "/my_queue"
#define SEM_FILL "/sem_fill"
#define SEM_EMPTY "/sem_empty"
#define SEM_MUTEX "/sem_mutex"


#define BUFFER_SIZE 6
#define SHM_SIZE (BUFFER_SIZE * sizeof(Dato) + sizeof(int))
#define SHM_SIZE_STRUCT (BUFFER_SIZE * sizeof(Dato))

#define MAX_MSG 7

#define MIN_LAG 0
#define MAX_LAG 10000

typedef struct
{
    bool fin;
    long objetivo;
    long solucion;
    bool correcto;
} Dato;

struct mq_attr
{
    long mq_flags;
    long mq_maxmsg;
    long mq_msgsize;
    long mq_curmsgs;
};

void number_range_error_handler(int min, int max, int value, char *msg)
{
    if (value < min)
    {
        fprintf(stderr, "%s can't be lower than %d\n", msg, min);
        exit(EXIT_FAILURE);
    }
    if (value > max)
    {
        fprintf(stderr, "%s can't be higher than %d\n", msg, max);
        exit(EXIT_FAILURE);
    }
    return;
}

void anadirElemento(Dato *bloque_shm, Dato *bloque_mq, int index)
{
    bloque_mq->fin = bloque_shm[index].fin;
    bloque_mq->objetivo = bloque_shm[index].objetivo;
    bloque_mq->solucion = bloque_shm[index].solucion;
    bloque_mq->correcto = bloque_shm[index].correcto;

    index = (index + 1) % BUFFER_SIZE;

    return;
}

Dato extraerElemento(Dato *bloque_shm, int index)
{

    Dato ret = NULL;

    bloque_shm[index].fin = ret.fin;
    bloque_shm[index].objetivo = ret.objetivo;
    bloque_shm[index].solucion = ret.solucion;
    bloque_shm[index].correcto = ret.correcto;

    index = (index - 1) % BUFFER_SIZE;

    return ret;
}

void monitor(int memory, int lag)
{
    Dato *bloque_shm;
    Dato bloque_monitor;
    int index;

    if ((bloque_shm = mmap(NULL, SHM_SIZE_STRUCT, PROT_READ | PROT_WRITE, MAP_SHARED, memory, 0)) == MAP_FAILED)
    {
        perror("mmap");
        close(bloque_shm);
        exit(EXIT_FAILURE);
    }

    if ((index = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, memory, 0)) == MAP_FAILED)
    {
        perror("mmap");
        close(index);
        exit(EXIT_FAILURE);
    }

    fprintf(stdout, "[%d] Printing blocks", getpid());

    while (1)
    {
        sem_wait(sem_fill);
        sem_wait(sem_mutex);
        bloque_monitor = extraerElemento(bloque_shm, index);
        sem_post(sem_mutex);
        sem_post(sem_empty);

        /* Imprime el bloque recibido por parte del minero*/
        if (bloque_monitor.correcto == false)
            fprintf(stdout, "Solution rejected: %08d !-> %08d\n", bloque_monitor.objetivo, bloque_monitor.solucion);

        fprintf(stdout, "Solution accepted: %08d --> %08d\n", bloque_monitor.objetivo, bloque_monitor.solucion);

        /* Comprobamos si hemos recibido el bloque de finalizacion */
        if(bloque_monitor.fin == true)
        {
            fprintf(stdout, "[%d] Finishing", getpid());
            close(bloque_shm);
            close(index);
            exit(EXIT_SUCCESS);
        }

        /*Realizamos espera de <LAG> milisegundos*/
        usleep(lag * 1000);
    }
    

    
}

void comprobador(int memory, unsigned int lag)
{

    mqd_t queue;
    Dato *bloque_shm;
    Dato bloque_mq;
    int bytes_read = 0;
    int index;
    unsigned int prio = 0;

    if ((bloque_shm = mmap(NULL, SHM_SIZE_STRUCT, PROT_READ | PROT_WRITE, MAP_SHARED, memory, 0)) == MAP_FAILED)
    {
        perror("mmap");
        close(bloque_shm);
        exit(EXIT_FAILURE);
    }

    if ((index = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, memory, 0)) == MAP_FAILED)
    {
        perror("mmap");
        close(index);
        exit(EXIT_FAILURE);
    }

    /* Abrimos la cola de mensajes, si no esta creada se crea */
    struct mq_attr attr = {0};
    attr.mq_maxmsg = MAX_MSG;
    attr.mq_msgsize = sizeof(Dato);

    /* Abrimos la cola de mensajes para leer y escribir */
    /* Si no la ha creado el minero retornamos error */
    queue = mq_open(QUEUE_NAME, O_RDONLY, 0);
    if (queue == -1)
    {
        perror("Error abriendo la cola de mensajes");
        close(bloque_shm);
        close(index);
        exit(EXIT_FAILURE);
    }
    printf("Abierta la cola de mensajes en el comprobador\n");

    fprintf(stdout, "[%d] Checking blocks", getpid());

    while (1)
    {

        /* Recibimos el bloque por parte del minero y lo guardamos en buffer */
        int bytes_read = mq_receive(queue, (char *)&bloque_mq, sizeof(Dato), &prio);
        if (bytes_read == -1)
        {
            perror("mq_receive");
            close(bloque_shm);
            close(index);
            mq_close(queue);
            exit(EXIT_FAILURE);
        }
        printf("Received message with priority %u: %s\n", priority, bloque_mq);

        /* Comprobamos si el bloque es correcto */
        if (pow_hash(bloque_mq->objetivo) == bloque_mq->solucion)
        {
            bloque_mq->correcto = true;
        }
        bloque_mq->correcto = false;

        /* Introducimos el bloque en memoria compartida para mandarlo al monitor */
        sem_wait(sem_empty);
        sem_wait(sem_mutex);
        anadirElemento(bloque_shm, bloque_mq, index);
        sem_post(sem_mutex);
        sem_post(sem_fill);

        /* Si era el bloque final finalizamos */
        if (bloque_mq->fin == true)
        {
            close(bloque_shm);
            close(index);
            mq_close(queue);
            fprintf(stdout, "[%d] Finishing", getpid());
            exit(EXIT_SUCCESS);
        }

        /*Realizamos espera de <LAG> milisegundos*/
        usleep(lag * 1000);
    }
}

int main(int argc, char **argv)
{
    int memory = 0;
    unsigned int lag;
    char *strptr;
    sem_t *sem_fill = NULL, *sem_empty = NULL, *sem_mutex = NULL;

    /* Control de errores num arguentos*/
    if (argc != 2)
    {
        printf("No se ha pasado el numero correcto de argumentos\n");
        printf("El formato correcto es:\n");
        printf("./monitor <LAG>\n");
        printf("El lag se medira en milisegundos, maximo 10000\n");

        exit(EXIT_FAILURE);
    }

    /* comprobamos que se nos ha pasado un numero como argumento */
    lag = (unsigned int)strtoul(argv[1], &strptr, 10);
    if (*strptr != '\0')
    {
        printf("Valor inválido: '%s' no es un numero\n", endptr);
        exit(EXIT_FAILURE);
    }
    number_range_error_handler(MIN_LAG, MAX_LAG, lag, "Lag");

    /* Abrimos los semaforos a utilizar de cara al manejo de la memoria compartida */
    if ((sem_fill = sem_open(SEM_FILL, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0)) == SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    if ((sem_empty = sem_open(SEM_EMPTY, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, BUFFER_SIZE)) == SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    if ((sem_mutex = sem_open(SEM_MUTEX, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1)) == SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    sem_unlink(SEM_FILL);
    sem_unlink(SEM_EMPTY);
    sem_unlink(SEM_MUTEX);

    memory = shm_open(SHM_NAME, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
    if (memory == -1)
    {
        /* Si la memoria compartida ya existe es el proceso monitor */
        if (errno == EEXIST)
        {
            memory = shm_open(SHM_NAME, O_RDONLY, 0);
            if (memory == -1)
            {
                perror("Error opening the shared memory segment");
                exit(EXIT_FAILURE);
            }
            else
            {
                printf("Shared memory segment open\n");
                shm_unlink(SHM_NAME);
                monitor(memory, lag);
            }
        }
        else
        {
            perror("Error creating the shared memory segment\n");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        /* Si la memoria compartida no existe la crea y se convierte en el comprobador */
        printf("Shared memory segment created\n");
        if (ftruncate(memory, SHM_SIZE) == -1)
        {
            perror("ftruncate");
            close(memory);
            exit(EXIT_FAILURE);
        }
        comprobador(memory, lag);
    }

    return EXIT_SUCCESS;
}