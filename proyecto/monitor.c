#include "common.h"

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

void anadirElemento(Dato *bloque_shm, Dato *bloque_mq)
{
    bloque_shm->fin = bloque_mq->fin;
    bloque_shm->objetivo = bloque_mq->objetivo;
    bloque_shm->solucion = bloque_mq->solucion;
    bloque_shm->correcto = bloque_mq->correcto;

    return;
}

Dato extraerElemento(Dato *bloque_shm)
{
    Dato ret;

    ret.fin = bloque_shm->fin;
    ret.objetivo = bloque_shm->objetivo;
    ret.solucion = bloque_shm->solucion;
    ret.correcto = bloque_shm->correcto;

    return ret;
}

void monitor(int memory, unsigned int lag)
{
    Bloque *bloque_shm;
    Dato bloque_monitor;
    sem_t *sem_fill = NULL, *sem_empty = NULL, *sem_mutex = NULL;

    /* Abrimos los semaforos a utilizar de cara al manejo de la memoria compartida */
    if ((sem_fill = sem_open(SEM_FILL, O_CREAT, S_IRUSR | S_IWUSR, 0)) == SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    if ((sem_empty = sem_open(SEM_EMPTY, O_CREAT, S_IRUSR | S_IWUSR, BUFFER_SIZE)) == SEM_FAILED)
    {
        perror("sem_open");
        sem_close(sem_fill);
        exit(EXIT_FAILURE);
    }

    if ((sem_mutex = sem_open(SEM_MUTEX, O_CREAT, S_IRUSR | S_IWUSR, 1)) == SEM_FAILED)
    {
        perror("sem_open");
        sem_close(sem_fill);
        sem_close(sem_empty);
        exit(EXIT_FAILURE);
    }

    sem_unlink(SEM_FILL);
    sem_unlink(SEM_EMPTY);
    sem_unlink(SEM_MUTEX);

    if ((bloque_shm = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, memory, 0)) == MAP_FAILED)
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
        if(bloque_monitor.fin == true)
        {
            printf("[%d] Finishing\n", getpid());
            close(memory);
            munmap(bloque_shm, SHM_SIZE);
            sem_close(sem_fill);
            sem_close(sem_empty);
            sem_close(sem_mutex);
            exit(EXIT_SUCCESS);
        }

        /* Imprime el bloque recibido por parte del minero*/
        if (bloque_monitor.correcto == false)
            printf("Solution rejected: %08ld !-> %08ld\n", bloque_monitor.objetivo, bloque_monitor.solucion);
        else
            printf("Solution accepted: %08ld --> %08ld\n", bloque_monitor.objetivo, bloque_monitor.solucion);


        /*Realizamos espera de <LAG> milisegundos*/
        usleep(lag * 1000);
    }
}

void comprobador(int memory, unsigned int lag)
{
    mqd_t queue;
    Bloque *bloque_shm;
    Dato bloque_mq;
    sem_t *sem_fill = NULL, *sem_empty = NULL, *sem_mutex = NULL;

    /* Abrimos los semaforos a utilizar de cara al manejo de la memoria compartida */
    if ((sem_fill = sem_open(SEM_FILL, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0)) == SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    if ((sem_empty = sem_open(SEM_EMPTY, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, BUFFER_SIZE)) == SEM_FAILED)
    {
        perror("sem_open");
        sem_close(sem_fill);
        exit(EXIT_FAILURE);
    }

    if ((sem_mutex = sem_open(SEM_MUTEX, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1)) == SEM_FAILED)
    {
        perror("sem_open");
        sem_close(sem_fill);
        sem_close(sem_empty);
        exit(EXIT_FAILURE);
    }

    
    if ((bloque_shm = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, memory, 0)) == MAP_FAILED)
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
        if (mq_receive(queue, (char *)&bloque_mq, sizeof(Dato), NULL) == -1)
        {
            perror("mq_receive");
            close(memory);
            munmap(bloque_shm, SHM_SIZE);
            mq_close(queue);
            exit(EXIT_FAILURE);
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
            close(memory);
            mq_close(queue);
            munmap(bloque_shm, SHM_SIZE);
            sem_close(sem_fill);
            sem_close(sem_empty);
            sem_close(sem_mutex);
            printf("[%d] Finishing\n", getpid());
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
        printf("Valor inválido: '%s' no es un numero\n", strptr);
        exit(EXIT_FAILURE);
    }
    number_range_error_handler(MIN_LAG, MAX_LAG, lag, "Lag");

    

    memory = shm_open(SHM_NAME, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
    if (memory == -1)
    {
        /* Si la memoria compartida ya existe es el proceso monitor */
        if (errno == EEXIST)
        {
            memory = shm_open(SHM_NAME, O_RDWR, 0);
            if (memory == -1)
            {
                perror("Error opening the shared memory segment");
                exit(EXIT_FAILURE);
            }
            else
            {
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