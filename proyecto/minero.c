#include "minero.h"

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
long target_search(long objective)
{
  long result = 0;

  for (long i = 0; i <= POW_LIMIT; i++)
  {

    result = pow_hash(i);
    if (result == objective)
    {
      return i;
    }
  }

  return -1;
}

int main(int argc, char **argv)
{

  pid_t minero_pid;
  int fd;
  char *fichero;

  /* Variables de memoria compartida */
  MemoriaMinero *mem;
  Bloque b_actual;
  Bloque b_ultimo;
  Bloque fin;
  Cartera *carteras_mineros;

  /* Variables de vbloque */
  long target = 0, solution = -1;

  /* Variables de terminal */
  int n_seconds = 0, n_threads = 0;

  /* Variables de tuberias */
  int minero_a_registrador[2];
  int pipe_status;
  ssize_t nbytes = 0;

  /* Variables de colas de mensajes */
  char *strptr;
  struct mq_attr attr;
  mqd_t queue;

  attr.mq_maxmsg = MAX_MSG;
  attr.mq_msgsize = sizeof(Bloque);
  attr.mq_curmsgs = 0;

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
  fin.fin = true;
  fin.objetivo = -1;
  fin.solucion = -1;
  fin.correcto = false;

  /* Formateamos argumentos de la terminal */
  n_seconds = atoi(argv[1]);
  number_range_error_handler(1, MAX_SECONDS, n_seconds, "Number of seconds");

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

    itoa(getppid(), fichero, 10);
    strcat(fichero, ".txt");

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
      nbytes = read(minero_a_registrador, b_actual, sizeof(b_actual));
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

        for (i = 0; i < (sizeof(b_actual.carteras) / sizeof(b_actual.carteras[0])); i++)
        {
          dprintf(fd, "%d:%u", b_actual.carteras[i].id_proceso, b_actual.carteras[i].monedas);
        }
      }

    } while (nbytes != 0);

    close(minero_a_registrador[0]);
    close(fd);

    exit(EXIT_SUCCESS);
  }
  else /* Es el proceso Minero */
  {
    /* Inicializamos la cola de mensajes */
    queue = mq_open(MQ_NAME, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, &attr);
    if (queue == -1)
    {
      if (errno == EEXIST)
      {
        queue = mq_open(MQ_NAME, O_RDWR, 0);
        if (queue == -1)
        {
          perror("Error opening the message queue");
          exit(EXIT_FAILURE);
        }
        printf("Message queue opened\n");
      }
      else
      {
        perror("Error creating the message queue\n");
        exit(EXIT_FAILURE);
      }
    }
    /* Cierra el extremo de lectura del minero */
    close(minero_a_registrador[0]);

    /**
     * El proceso Minero abre el segmento de memoria compartida con la
     * información del sistema, detectando si es el primero minero o no
     */
    memoria = shm_open(SHM_NAME, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
    if (memoria == -1)
    {
      /* Un minero que no es el primero abre la memoria compartida */
      if (errno == EEXIST)
      {
        memoria = shm_open(SHM_NAME, O_RDWR, 0);
        if (memoria == -1)
        {
          perror("Error abriendo el segmento de memoria compartida");
          exit(EXIT_FAILURE);
        }
        else
        {
          shm_unlink(SHM_NAME);
        }
      }
    }
    else
    {
      /* Es el primer minero y hace los preparativos necesarios */

      if (ftruncate(mem, SHM_SIZE_MINERO) == -1)
      {
        perror("ftruncate");
        close(mem);
        exit(EXIT_FAILURE);
      }

      mem = mmap(NULL, SHM_SIZE_MINERO, PROT_READ | PROT_WRITE, MAP_SHARED, mem, 0);
      mem->mineros = NULL;
      mem->votos = NULL;
      mem->monedas = NULL;

      carteras_mineros = calloc(1, sizeof(Cartera)) if (carteras_mineros == NULL)
      {
        printf("Error al asignar memoria a las carteras\n");
        return EXIT_FAILURE;
      }

      /* Inicializacion del bloque ultimo */
      b_ultimo.votos_positivos = 0;
      b_ultimo.votos_totales = 0;
      b_ultimo.objetivo = -1;
      b_ultimo.solucion = -1;
      b_ultimo.id = -1;
      b_ultimo.carteras = NULL;
      mem->bl_ultimo = b_ultimo;

      /* Inicializacion del primer bloque de minado */
      b_actual.votos_positivos = 0;
      b_actual.votos_totales = 0;
      b_actual.objetivo = -1;
      b_actual.solucion = -1;
      b_actual.id = -1;
      b_actual.carteras = NULL;
      mem->bl_actual = b_actual;
    }
  }

  /**
   * @brief Create and manage threads that will search the solution of the POW function.
   *
   * @param n_threads Number of threads to manage.
   * @param target
   * @return Exit of the thread.
   */
  int manage_threads(int n_threads, int target)
  {
    int solution = -1;
    int error;
    pthread_t *threads;
    search objective[n_threads];

    target_found = NOTFOUND;

    if (!(threads = (pthread_t *)calloc(sizeof(pthread_t), n_threads)))
    {
      fprintf(stderr, "Error allocating memory of threads");
      return EXIT_FAILURE;
    }

    for (int i = 0; i < n_threads; i++)
    {
      /* Establish the search range of the trhead and the target */
      objective[i].min = POW_LIMIT / n_threads * i;
      objective[i].max = (POW_LIMIT / n_threads * (i + 1)) - 1;
      objective[i].target = target;
      objective[i].solution = -1;
      error = pthread_create(&threads[i], NULL, target_search, &objective[i]);
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

      if (objective[i].solution != -1)
      {
        solution = objective[i].solution;
      }
    }

    free(threads);

    return solution;
  }

  for (int i = 0; i < n_cycles; i++)
  {
    solution = target_search(target);

    if (solution == -1)
    {
      printf("Error on miner process\n");
      return EXIT_FAILURE;
    }

    resultado.fin = false;
    resultado.objetivo = target;
    resultado.solucion = solution;
    resultado.correcto = true;

    if (mq_send(queue, (char *)&resultado, sizeof(Dato), 1) == -1)
    {
      perror("mq_send");
      mq_close(queue);
      exit(EXIT_FAILURE);
    }

    target = solution;

    usleep(lag * 1000);
  }

  /* All rounds finished, notifying the monitor */
  solution = -1;
  if (mq_send(queue, (char *)&fin, sizeof(Dato), 1) == -1)
  {
    perror("mq_send");
    mq_close(queue);
    exit(EXIT_FAILURE);
  }

  mq_close(queue);

  printf("[%d] Finishing\n", getpid());

  return EXIT_SUCCESS;
}