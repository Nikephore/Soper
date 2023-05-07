/**
 * @file monitor.c
 * @author Luis Miguel Nucifora & Alexis Canales Molina.
 * @brief Code for the monitor, who will check the answers from the miners and print them
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "common.h"

static volatile sig_atomic_t sint = 0;

/**
 * @brief Handler for SIGINT
 *
 * @param sig signal to be treated
 * @return void
 */
void catcher(int sig)
{
    switch (sig)
    {
    case SIGINT:
        sint = 1;
        break;
    }
}

/**
 * @brief Puts a block in the shared memory
 *
 * @param bloque_shm pointer to the zone of the shared memoryfor it to be introduced
 * @param bloque_mp pointer to the block to be introduced
 * @return void
 */
void anadirElemento(Bloque *bloque_shm, Bloque *bloque_mq)
{
    bloque_shm->id = bloque_mq->id;
    bloque_shm->ganador = bloque_mq->ganador;
    memcpy(bloque_shm->carteras, bloque_mq->carteras, sizeof(Cartera) * MAX_MINEROS);

    bloque_shm->votos_totales = bloque_mq->votos_totales;
    bloque_shm->votos_positivos = bloque_mq->votos_positivos;
    bloque_shm->objetivo = bloque_mq->objetivo;
    bloque_shm->solucion = bloque_mq->solucion;
    bloque_shm->correcto = bloque_mq->correcto;
    bloque_shm->fin = bloque_mq->fin;

    return;
}

/**
 * @brief Extracts a block from the shared memory
 *
 * @param bloque_shm pointer to the block to be extracted
 * @return the extracted block
 */
Bloque extraerElemento(Bloque *bloque_shm)
{
    Bloque ret;

    ret.id = bloque_shm->id;
    ret.ganador = bloque_shm->ganador;
    memcpy(ret.carteras, bloque_shm->carteras, sizeof(Cartera) * MAX_MINEROS);
    ret.votos_totales = bloque_shm->votos_totales;
    ret.votos_positivos = bloque_shm->votos_positivos;
    ret.objetivo = bloque_shm->objetivo;
    ret.solucion = bloque_shm->solucion;
    ret.correcto = bloque_shm->correcto;
    ret.fin = bloque_shm->fin;

    return ret;
}

/**
 * @brief Prints a block in standard output
 *
 * @param bloque block to be printed
 * @return void
 */
void imprimir_bloque(Bloque bloque)
{
    int i = 0, tam = 0;

    printf("Id:\t\t%04d\n", bloque.id);
    printf("Winner:\t%d\n", bloque.ganador);
    printf("Target:\t%ld\n", bloque.objetivo);
    printf("Solution:\t%ld ", bloque.solucion);
    if (bloque.correcto == true)
        printf("(validated)\n");
    else
        printf(("rejected)\n"));
    printf("Votes:\t%d/%d\n", bloque.votos_positivos, bloque.votos_totales);
    printf("Wallets:\t");

    tam = sizeof(bloque.carteras) / sizeof(bloque.carteras[0]);

    for (i = 0; i < tam; i++)
    {
        printf("%d:%02u\t", bloque.carteras[i].id_proceso, bloque.carteras[i].monedas);
    }
    printf("\n\n");
}

/**
 * @brief Monitor which does the main functionality of the Monitor, including:
 *    receiving messages and printing them
 *
 * @param memory file descriptor to shared memory
 * @param sem_fill fill semaphore
 * @param sem_empty empty semaphore
 * @param sem_mutex mutex semaphore
 * @return EXIT_SUCCESS if everything went right, EXIT_FAILURE otherwise
 */
void monitor(int memory, sem_t *sem_fill, sem_t *sem_empty, sem_t *sem_mutex)
{
    MemoriaMonitor *bloque_shm;
    Bloque bloque_monitor;

    if ((bloque_shm = mmap(NULL, SHM_SIZE_MONITOR, PROT_READ | PROT_WRITE, MAP_SHARED, memory, 0)) == MAP_FAILED)
    {
        perror("mmap");
        close(memory);
        sem_close(sem_fill);
        sem_close(sem_empty);
        sem_close(sem_mutex);
        exit(EXIT_FAILURE);
    }

    printf("[%d] Printing blocks...\n", getpid());

    while (1)
    {
        sem_wait(sem_fill);
        sem_wait(sem_mutex);
        bloque_monitor = extraerElemento(&bloque_shm->bloque[bloque_shm->front]);
        bloque_shm->front = (bloque_shm->front + 1) % BUFFER_SIZE;
        sem_post(sem_mutex);
        sem_post(sem_empty);

        /* Comprobamos si hemos recibido el bloque de finalizacion */
        if (bloque_monitor.fin == true)
        {
            printf("[%d] Finishing\n", getpid());
            close(memory);
            munmap(bloque_shm, SHM_SIZE_MONITOR);
            sem_close(sem_fill);
            sem_close(sem_empty);
            sem_close(sem_mutex);
            exit(EXIT_SUCCESS);
        }

        /* Imprime el bloque recibido por parte del minero*/
        imprimir_bloque(bloque_monitor);
    }
}

/**
 * @brief Comprobador which does the main functionality of the Comprobador, including:
 *    opening the message queue, receiving messages, checking them 
 *    and putting them in shared memory
 *
 * @param memory file descriptor to shared memory
 * @param sem_fill fill semaphore
 * @param sem_empty empty semaphore
 * @param sem_mutex mutex semaphore
 * @return EXIT_SUCCESS if everything went right, EXIT_FAILURE otherwise
 */
void comprobador(int memory, sem_t *sem_fill, sem_t *sem_empty, sem_t *sem_mutex)
{
    mqd_t queue;
    MemoriaMonitor *bloque_shm;
    Bloque bloque_mq;
    struct sigaction sact;

    /* Inicializacion de señales */
    sigemptyset(&sact.sa_mask);
    sact.sa_flags = 0;
    sact.sa_handler = catcher;

    if (sigaction(SIGINT, &sact, NULL) != 0)
        perror("sigaction() SIGINT error");

    if ((bloque_shm = mmap(NULL, SHM_SIZE_MONITOR, PROT_READ | PROT_WRITE, MAP_SHARED, memory, 0)) == MAP_FAILED)
    {
        perror("mmap");
        close(memory);
        sem_close(sem_fill);
        sem_close(sem_empty);
        sem_close(sem_mutex);
        exit(EXIT_FAILURE);
    }
    bloque_shm->front = 0;
    bloque_shm->rear = 0;

    /* Abrimos la cola de mensajes para leer y escribir */
    /* Si no la ha creado el minero retornamos error */
    queue = mq_open(MQ_NAME, O_RDONLY, 0);
    if (queue == -1)
    {
        perror("Error abriendo la cola de mensajes");
        close(memory);
        exit(EXIT_FAILURE);
    }
    mq_unlink(MQ_NAME);
    printf("[%d] Checking blocks...\n", getpid());

    while (1)
    {
        /* Recibimos el bloque por parte del minero y lo guardamos en buffer */
        if (mq_receive(queue, (char *)&bloque_mq, sizeof(Bloque), NULL) == -1)
        {
            perror("mq_receive");
            /* Enviar bloque de finalizacion en caso de error */
            bloque_mq.fin = true;
            sem_wait(sem_empty);
            sem_wait(sem_mutex);
            anadirElemento(&bloque_shm->bloque[bloque_shm->rear], &bloque_mq);
            bloque_shm->rear = (bloque_shm->rear + 1) % BUFFER_SIZE;
            sem_post(sem_mutex);
            sem_post(sem_fill);
            wait(NULL);

            munmap(bloque_shm, SHM_SIZE_MONITOR);
            close(memory);
            exit(EXIT_FAILURE);
        }

        /* Hemos recibido SIGINT, finalizamos */
        if (sint == 1)
        {
            /* Enviar bloque de finalizacion en caso de error */
            bloque_mq.fin = true;
            sem_wait(sem_empty);
            sem_wait(sem_mutex);
            anadirElemento(&bloque_shm->bloque[bloque_shm->rear], &bloque_mq);
            bloque_shm->rear = (bloque_shm->rear + 1) % BUFFER_SIZE;
            sem_post(sem_mutex);
            sem_post(sem_fill);

            wait(NULL);
            munmap(bloque_shm, SHM_SIZE_MONITOR);
            close(memory);
            printf("[%d] Finishing for \n", getpid());
            exit(EXIT_SUCCESS);
        }

        /* Comprobamos si el bloque es correcto */
        if (pow_hash(bloque_mq.solucion) != bloque_mq.objetivo)
        {
            bloque_mq.correcto = false;
        }
        else bloque_mq.correcto = true;

        /* Introducimos el bloque en memoria compartida para mandarlo al monitor */
        sem_wait(sem_empty);
        sem_wait(sem_mutex);
        anadirElemento(&bloque_shm->bloque[bloque_shm->rear], &bloque_mq);
        bloque_shm->rear = (bloque_shm->rear + 1) % BUFFER_SIZE;
        sem_post(sem_mutex);
        sem_post(sem_fill);

        /* Si era el bloque final finalizamos */
        if (bloque_mq.fin == true)
        {
            wait(NULL);
            close(memory);
            munmap(bloque_shm, SHM_SIZE_MONITOR);
            printf("[%d] Finishing\n", getpid());
            exit(EXIT_SUCCESS);
        }
    }
}

/**
 * @brief Main which does the main functionality of the monitor, including:
 *    creation of the child Monitor and initializing shared memory
 *
 * @return EXIT_SUCCESS if everything went right, EXIT_FAILURE otherwise
 */
int main()
{
    int memory = 0;
    pid_t comprobador_pid;

    /* Semaforos sin nombre */
    sem_t *sem_fill = NULL;
    sem_t *sem_empty = NULL;
    sem_t *sem_mutex = NULL;
    
    memory = shm_open(SHM_MONITOR, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
    if (memory == -1)
    {
        if (errno == EEXIST)
        {
            perror("shm_open");
            close(memory);
            shm_unlink(SHM_MONITOR);
        }
        exit(EXIT_FAILURE);
    }
    else
    {
        shm_unlink(SHM_MONITOR);
        if (ftruncate(memory, SHM_SIZE_MONITOR) == -1)
        {
            perror("ftruncate");
            close(memory);
            exit(EXIT_FAILURE);
        }
    }

    /* Inicializacion de semaforos */
    if ((sem_fill = sem_open(SEM_FILL, O_CREAT | O_EXCL , S_IRUSR | S_IWUSR, 0)) == SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    if ((sem_empty = sem_open(SEM_EMPTY, O_CREAT | O_EXCL , S_IRUSR | S_IWUSR, BUFFER_SIZE)) == SEM_FAILED)
    {
        perror("sem_open");
        sem_close(sem_fill);
        exit(EXIT_FAILURE);
    }

    if ((sem_mutex = sem_open(SEM_MUTEX, O_CREAT | O_EXCL , S_IRUSR | S_IWUSR, 1)) == SEM_FAILED)
    {
        perror("sem_open");
        sem_close(sem_fill);
        sem_close(sem_empty);
        exit(EXIT_FAILURE);
    }

    sem_unlink(SEM_FILL);
    sem_unlink(SEM_EMPTY);
    sem_unlink(SEM_MUTEX);

    comprobador_pid = fork();
    pid_error_handler(comprobador_pid);

    if (comprobador_pid == 0) /* Proceso monitor */
    {
        monitor(memory, sem_fill, sem_empty, sem_mutex);
    }
    else /* Proceso comprobador */
    {
        comprobador(memory, sem_fill, sem_empty, sem_mutex);
    }

    return EXIT_SUCCESS;
}