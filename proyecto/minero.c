#include "minero.h"

static volatile sig_atomic_t sint = 0;
static volatile sig_atomic_t salarm = 0;

void catcher(int sig)
{
  switch (sig)
  {
  case SIGINT:
    sint = 1;
    break;
  case SIGALRM:
    salarm = 1;
    break;
  }
}

/**
 * @brief Comprueba si ha habido algun error en la creacion de una tuberia
 *
 * @param estado de la tuberia
 */
void pipe_error_handler(int pipe_status)
{
  if (pipe_status == -1)
  {
    perror("pipe");
    exit(EXIT_FAILURE);
  }
}

/**
 * @brief Search the correct value of the pow function between a specified range.
 *
 * @param objective Pointer to structure with the range values and the target to search.
 * @return Exit of the thread.
 */
void *target_search(void *busqueda)
{

  Busqueda *obj = busqueda;
  int result;
  int target_found = NOT_FOUND;

  for (int i = obj->min; i <= obj->max; i++)
  {

    if (target_found == FOUND)
    {
      pthread_exit((void *)obj);
    }

    result = pow_hash(i);
    if (result == obj->objetivo)
    {
      target_found = FOUND;
      obj->solucion = i;
      pthread_exit((void *)obj);
    }
  }

  pthread_exit((void *)obj);
}

/**
 * @brief Create and manage threads that will search the solution of the POW function.
 *
 * @param n_threads Number of threads to manage.
 * @param bloque
 * @return Modified block.
 */
long manage_threads(int n_threads, long objetivo)
{
  long solucion = -1;
  int error;
  pthread_t *threads;
  Busqueda busqueda[n_threads];

  if (!(threads = (pthread_t *)calloc(sizeof(pthread_t), n_threads)))
  {
    fprintf(stderr, "Error allocating memory of threads");
    return EXIT_FAILURE;
  }

  for (int i = 0; i < n_threads; i++)
  {
    /* Establish the search range of the trhead and the target */
    busqueda[i].min = (int)((POW_LIMIT / (double)n_threads) * i);
    busqueda[i].max = (int)(((POW_LIMIT / (double)n_threads) * (i + 1)) - 1);
    busqueda[i].objetivo = objetivo;
    busqueda[i].solucion = -1;
    error = pthread_create(&threads[i], NULL, target_search, &busqueda[i]);
    if (error != 0)
    {
      fprintf(stderr, "pthread_create: %s\n", strerror(error));
      return EXIT_FAILURE;
    }
  }

  for (int i = 0; i < n_threads; i++)
  {
    error = pthread_join(threads[i], NULL);
    if (error != 0)
    {
      fprintf(stderr, "pthread_join: %s\n", strerror(error));
      return EXIT_FAILURE;
    }

    if (busqueda[i].solucion != -1)
    {
      solucion = busqueda[i].solucion;
    }
  }

  free(threads);

  return solucion;
}

int main(int argc, char **argv)
{

  pid_t minero_pid;
  int fd;
  char fichero[MAX_FILE_SIZE] = ""; /* fichero para el registrador */
  int n_minero = 0;

  /* Variables de memoria compartida */
  int memoria;
  MemoriaMinero *mem;
  Bloque b_actual;
  Bloque b_ultimo;
  Bloque auxiliar;
  Bloque fin;

  /* Variables de bloque */
  long solucion = -1;
  long objetivo = 0;

  /* Variables de terminal */
  int n_seconds = 0, n_threads = 0;

  /* Variables de tuberias */
  int minero_a_registrador[2];
  int pipe_status;
  ssize_t nbytes = 0;

  /* Variables de colas de mensajes */
  struct mq_attr attr;
  mqd_t queue;

  struct sigaction sact;

  /* Control de errores num arguentos*/
  if (argc != 3)
  {
    printf("No se ha pasado el numero correcto de argumentos\n");
    printf("El formato correcto es:\n");
    printf("./miner <N_SECONDS> <N_THREADS>\n");
    printf("\n");

    exit(EXIT_FAILURE);
  }

  /* Inicializamos los atributos del bloque final*/
  fin.id = -1;
  fin.objetivo = -1;
  fin.solucion = -1;
  fin.ganador = 0;
  fin.votos_totales = 0;
  fin.votos_positivos = 0;
  fin.correcto = false;
  fin.fin = true;

  auxiliar.id = -1;
  auxiliar.objetivo = -1;
  auxiliar.solucion = -1;
  auxiliar.ganador = 0;
  auxiliar.votos_totales = 0;
  auxiliar.votos_positivos = 0;
  auxiliar.correcto = false;
  auxiliar.fin = false;

  /* Formateamos argumentos de la terminal */
  n_seconds = atoi(argv[1]);
  number_range_error_handler(1, MAX_SECONDS, n_seconds, "Number of seconds");

  alarm(n_seconds);

  n_threads = atoi(argv[2]);
  number_range_error_handler(1, MAX_THREADS, n_threads, "Number of threads");

  /* [0] is to Read | [1] is to Write */
  pipe_status = pipe(minero_a_registrador);
  pipe_error_handler(pipe_status);

  minero_pid = fork();
  pid_error_handler(minero_pid);

  /* Es el proceso Registrador*/
  if (minero_pid == 0)
  {
    sprintf(fichero, "%d.txt", getppid());

    fd = open(fichero, O_CREAT | O_WRONLY | O_APPEND);
    if (fd == -1)
    {
      perror("open");
      exit(EXIT_FAILURE);
    }

    /* Cierra el extremo de escritura del registrador */
    close(minero_a_registrador[1]);

    do
    {
      nbytes = read(minero_a_registrador[0], (char *)&b_actual, sizeof(b_actual));
      if (nbytes == -1)
      {
        perror("read");
        exit(EXIT_FAILURE);
      }

      if (nbytes > 0)
      {
        dprintf(fd, "id:\t%d\nWinner:\t%d\nTarget:\t%ld\nSolution:\t%ld\t", b_actual.id, b_actual.ganador, b_actual.objetivo, b_actual.solucion);

        if (b_actual.votos_positivos >= ceil(b_actual.votos_totales / 2))
        {
          dprintf(fd, "(validated)\n");
        }
        else
        {
          dprintf(fd, "(rejected)\n");
        }

        dprintf(fd, "Votes:\t%u/%u\nWallets:\t", b_actual.votos_positivos, b_actual.votos_totales);

        for (int i = 0; i < (int)(sizeof(b_actual.carteras) / sizeof(b_actual.carteras[0])); i++)
        {
          dprintf(fd, "%d:%u ", b_actual.carteras[i].id_proceso, b_actual.carteras[i].monedas);
        }

        dprintf(fd, "\n\n");
      }

    } while (nbytes != 0);

    close(minero_a_registrador[0]);
    close(fd);

    exit(EXIT_SUCCESS);
  }
  else /* Es el proceso Minero */
  {

    /* Inicializacion de señales */
    sigemptyset(&sact.sa_mask);
    sact.sa_flags = 0;
    sact.sa_handler = catcher;

    if (sigaction(SIGINT, &sact, NULL) != 0)
      perror("sigaction() SIGINT error");

    if (sigaction(SIGALRM, &sact, NULL) != 0)
      perror("sigaction() SIGALRM error");


    /* Cierra el extremo de lectura del minero */
    close(minero_a_registrador[0]);

    /* Inicializamos la cola de mensajes */
    attr.mq_flags = O_CREAT | O_NONBLOCK;
    attr.mq_maxmsg = MAX_MSG;
    attr.mq_msgsize = sizeof(Bloque);
    attr.mq_curmsgs = 0;

    queue = mq_open(MQ_NAME, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, &attr);
    if (queue == -1)
    {
      if (errno == EEXIST)
      {
        perror("Error opening the message queue");
        mq_unlink(MQ_NAME);
        close(minero_a_registrador[1]);
        wait(NULL);
        exit(EXIT_FAILURE);
      }
      else
      {
        close(minero_a_registrador[1]);
        wait(NULL);
        perror("Error creating the message queue\n");
        exit(EXIT_FAILURE);
      }
    }
    /**
     * El proceso Minero abre el segmento de memoria compartida con la
     * información del sistema, detectando si es el primero minero o no
     */
    memoria = shm_open(SHM_MINERO, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
    if (memoria == -1)
    {
      /* Un minero que no es el primero abre la memoria compartida */
      if (errno == EEXIST)
      {
        close(memoria); /* Cerrar memoria */
        shm_unlink(SHM_MINERO);
      }

      mq_close(queue);
      close(minero_a_registrador[1]); /* Cerrar tuberia */
      wait(NULL);
      exit(EXIT_FAILURE);
    }

    /* Es el primer minero y hace los preparativos necesarios */
    if (ftruncate(memoria, SHM_SIZE_MINERO) == -1)
    {
      perror("ftruncate");
      close(memoria);
      shm_unlink(SHM_MINERO);
      wait(NULL);
      exit(EXIT_FAILURE);
    }

    /* Realizamos el mapeo de la memoria y de los punteros de la estructura */
    mem = mmap(NULL, SHM_SIZE_MINERO, PROT_READ | PROT_WRITE, MAP_SHARED, memoria, 0);

    mem->num_mineros = 1;
    n_minero = mem->num_mineros;

    /* Inicializacion del bloque ultimo */
    b_ultimo.votos_positivos = 0;
    b_ultimo.votos_totales = 0;
    b_ultimo.objetivo = -1;
    b_ultimo.solucion = -1;
    b_ultimo.id = -1;
    b_actual.carteras[mem->num_mineros - 1].id_proceso = getpid();
    b_actual.carteras[mem->num_mineros - 1].monedas = 0;
    mem->bl_ultimo = b_ultimo;

    /* Inicializacion del primer bloque de minado */
    b_actual.votos_positivos = 0;
    b_actual.votos_totales = 0;
    b_actual.objetivo = 0;
    b_actual.solucion = -1;
    b_actual.id = 1;
    b_actual.carteras[mem->num_mineros - 1].id_proceso = getpid();
    b_actual.carteras[mem->num_mineros - 1].monedas = 0;
    b_actual.correcto = false;
    b_actual.fin = false;
    mem->bl_actual = b_actual;

    if (mem->num_mineros == MAX_MINEROS - 1)
    {
      munmap(mem, SHM_SIZE_MINERO);
      close(memoria);
      mq_close(queue);
      close(minero_a_registrador[1]);
      wait(NULL);
      exit(EXIT_FAILURE);
    }

    mem->num_mineros++;

    /* Mientras no se hayan recibido alarmas minamos bloques */
    while (sint != 1 && salarm != 1)
    {
      sigaddset(&sact.sa_mask, SIGALRM);
      sigaddset(&sact.sa_mask, SIGINT);

      objetivo = mem->bl_actual.objetivo;
      solucion = manage_threads(n_threads, objetivo);
      /* Soy el minero ganador de la ronda (y el unico)*/
      mem->bl_actual.solucion = solucion;
      mem->bl_actual.ganador = getpid();

      /* Ronda de votacion simple con un minero */
      if (pow_hash(solucion) == objetivo)
      {
        mem->bl_actual.votos_positivos++;
      }
      mem->bl_actual.votos_totales++;

      /* Votacion terminada, comprobando votos */
      if (mem->bl_actual.votos_positivos >= ceil(mem->bl_actual.votos_totales / 2))
      {
        printf("GANO MONEDA\n");
        mem->bl_actual.carteras[n_minero-1].monedas++;
      }

      auxiliar = mem->bl_actual;

      /* Mando por tuberia el bloque de memoria completo a mi registrador */
      nbytes = write(minero_a_registrador[1], (char *)&auxiliar, sizeof(Bloque));
      if (nbytes == -1)
      {
        perror("write");
        close(minero_a_registrador[1]);
        mq_close(queue);
        munmap(mem, SHM_SIZE_MINERO);
        close(memoria);
        wait(NULL);
        exit(EXIT_FAILURE);
      }

      if (mq_getattr(queue, &attr) == -1)
      {
        perror("Error al obtener los atributos de la cola de mensajes");
        exit(EXIT_FAILURE);
      }

      if (attr.mq_curmsgs < MAX_MSG)
      {
        /* Envio por la cola de mensajes el bloque al monitor*/
        if (mq_send(queue, (char *)&auxiliar, sizeof(Bloque), 1) == -1)
        {
          if (errno != EAGAIN)
          {
            perror("mq_send");
            mq_close(queue);
            munmap(mem, SHM_SIZE_MINERO);
            close(memoria);
            close(minero_a_registrador[1]);
            wait(NULL);
            exit(EXIT_FAILURE);
          }
        }
        
      }

      /* El bloque actual pasa a ser el ultimo */

      mem->bl_ultimo.id = mem->bl_actual.id;
      mem->bl_ultimo.objetivo = mem->bl_actual.objetivo;
      mem->bl_ultimo.solucion = mem->bl_actual.solucion;
      mem->bl_ultimo.ganador = mem->bl_actual.ganador;
      mem->bl_ultimo.votos_totales = mem->bl_actual.votos_totales;
      mem->bl_ultimo.votos_positivos = mem->bl_actual.votos_positivos;
      mem->bl_ultimo.correcto = mem->bl_actual.correcto;
      mem->bl_ultimo.fin = mem->bl_actual.fin;

      mem->bl_actual.id++;
      mem->bl_actual.votos_positivos = 0;
      mem->bl_actual.votos_totales = 0;
      mem->bl_actual.ganador = -1;
      mem->bl_actual.objetivo = mem->bl_actual.solucion;
      mem->bl_actual.solucion = -1;
      mem->bl_actual.correcto = false;
      mem->bl_actual.fin = false;

      sigdelset(&sact.sa_mask, SIGINT);
      sigdelset(&sact.sa_mask, SIGALRM);

    }

    /* Fin del while, hemos recibido SIGINT o SIGALARM */
    printf("HE RECIBIDO SIGINT O SIGALARM\n");

    /* Envio por la cola de mensajes el bloque de finalizacion al monitor*/
    if (mq_getattr(queue, &attr) == -1)
    {
      perror("Error al obtener los atributos de la cola de mensajes");
      exit(EXIT_FAILURE);
    }

    if (attr.mq_curmsgs < MAX_MSG)
    {
      if (mq_send(queue, (char *)&fin, sizeof(Bloque), 1) == -1)
      {
        if (errno != EAGAIN)
        {
          perror("mq_send");
          mq_close(queue);
          munmap(mem, SHM_SIZE_MINERO);
          close(memoria);
          close(minero_a_registrador[1]);
          wait(NULL);
          exit(EXIT_FAILURE);
        }
      }
    }

    shm_unlink(SHM_MINERO);
    mem->num_mineros--;
    mq_unlink(MQ_NAME);
    close(minero_a_registrador[1]);
    munmap(mem, SHM_SIZE_MINERO);
    close(memoria);
    mq_close(queue);
    wait(NULL);

    exit(EXIT_SUCCESS);
  }
}
