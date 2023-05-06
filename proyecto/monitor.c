#include "common.h"

static volatile sig_atomic_t sint = 0;

void catcher(int sig)
{
    switch (sig)
    {
    case SIGUSR1:
        break;
    case SIGUSR2:
        break;
    case SIGINT:
        sint = 1;
        break;
    case SIGALRM:
        salarm = 1;
        break;
    }
}

void anadirElemento(Dato *bloque_shm, Dato *bloque_mq)
{
    bloque_shm->fin = bloque_mq->fin;
    bloque_shm->objetivo = bloque_mq->objetivo;
    bloque_shm->solucion = bloque_mq->solucion;
    bloque_shm->correcto = bloque_mq->correcto;

    return;
}

/*
Dato extraerElemento(Dato *bloque_shm)
{
    Dato ret;

    ret.fin = bloque_shm->fin;
    ret.objetivo = bloque_shm->objetivo;
    ret.solucion = bloque_shm->solucion;
    ret.correcto = bloque_shm->correcto;

    return ret;
}
*/

void imprimir_bloque(Bloque bloque)
{
    int i = 0;

    printf("Id:\t\t%04d\n", bloque.id);
    printf("Winner:\t%d\n",bloque.ganador);
    printf("Target:\t%ld\n",bloque.objetivo);
    printf("Solution:\t%ld ",bloque.solucion);
    if(bloque.votos_positivos >= ceil(bloque.votos_totales/2))
        printf("(validated)\n");
    else
        printf(("rejected)\n"));
    printf("Votes:\t%d/%d\n", bloque.votos_positivos, bloque.votos_totales)
    printf("Wallets:\t");

    for (i = 0; i < (sizeof(b_actual.carteras) / sizeof(b_actual.carteras[0])); i++)
    {
        printf("%d:%02u\t", bloque.carteras[i].id_proceso, bloque.carteras[i].monedas);
    }
    printf("\n\n");
}


void monitor(int memory, sem_t *sem_fill, sem_t *sem_empty, sem_t *sem_mutex)
{
    MemoriaMonitor *bloque_shm;
    Bloque bloque_monitor;


    if ((bloque_shm = mmap(NULL, SHM_SIZE_MONITOR, PROT_READ | PROT_WRITE, MAP_SHARED, memory, 0)) == MAP_FAILED)
    {
        perror("mmap");
        close(memory);
        sem_destroy(sem_fill);
        sem_destroy(sem_empty);
        sem_destroy(sem_mutex);
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
        if(bloque_monitor.fin == true)
        {
            printf("[%d] Finishing\n", getpid());
            close(memory);
            munmap(bloque_shm, SHM_SIZE_MONITOR);
            sem_destroy(sem_fill);
            sem_destroy(sem_empty);
            sem_destroy(sem_mutex);
            exit(EXIT_SUCCESS);
        }

        /* Imprime el bloque recibido por parte del minero*/
        imprimir_bloque(bloque);

    }
}

void comprobador(int memory, sem_t *sem_fill, sem_t *sem_empty, sem_t *sem_mutex)
{
    mqd_t queue;
    MemoriaMonitor *bloque_shm;
    Bloque bloque_mq;

    if ((bloque_shm = mmap(NULL, SHM_SIZE_MONITOR, PROT_READ | PROT_WRITE, MAP_SHARED, memory, 0)) == MAP_FAILED)
    {
        perror("mmap");
        close(memory);
        sem_destroy(sem_fill);
        sem_destroy(sem_empty);
        sem_destroy(sem_mutex);
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
            wait();

            munmap(bloque_shm, SHM_SIZE_MONITOR);
            close(memory);
            exit(EXIT_FAILURE);
        }

        /* Hemos recibido SIGINT, finalizamos */
        if(sint == 1) 
        {
            /* Enviar bloque de finalizacion en caso de error */
            bloque_mq.fin = true;
            sem_wait(sem_empty);
            sem_wait(sem_mutex);
            anadirElemento(&bloque_shm->bloque[bloque_shm->rear], &bloque_mq);
            bloque_shm->rear = (bloque_shm->rear + 1) % BUFFER_SIZE;
            sem_post(sem_mutex);
            sem_post(sem_fill);

            wait();
            munmap(bloque_shm, SHM_SIZE_MONITOR);
            close(memory);
            printf("[%d] Finishing for \n", getpid());
            exit(EXIT_SUCCESS);
        }

        /* Comprobamos si el bloque es correcto */
        if (pow_hash(bloque_mq.solucion) != bloque_mq.objetivo)
        {
            bloque_mq.correcto = false;
        } else bloque_mq.correcto = true;

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
            wait();
            close(memory);
            munmap(bloque_shm, SHM_SIZE_MONITOR);
            printf("[%d] Finishing\n", getpid());
            exit(EXIT_SUCCESS);
        }

    }
}

int main(int argc, char **argv)
{
    int memory = 0;
    pid_t comprobador_pid;
    char *strptr;

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
    if (sem_init(sem_fill, 0, 0) == -1)
    {
        perror("sem_init");
        exit(EXIT_FAILURE);
    }

    if (sem_init(sem_empty, 0, BUFFER_SIZE) == -1)
    {
        perror("sem_init");
        sem_destroy(sem_fill);
        exit(EXIT_FAILURE);
    }

    if (sem_init(&bloque_shm->sem_mutex, 0, 1) == -1)
    {
        perror("sem_init");
        sem_destroy(&bloque_shm->sem_fill);
        sem_destroy(&bloque_shm->sem_empty);
        exit(EXIT_FAILURE);
    }

    comprobador_pid = fork();
    pid_error_handler(comprobador_pid);

    if(comprobador_pid == 0) /* Proceso monitor */
    {
        monitor(memory, sem_fill, sem_empty, sem_mutex);
    }
    else /* Proceso comprobador */
    {
        comprobador(memory, sem_fill, sem_empty, sem_mutex);
    }

    return EXIT_SUCCESS;
}