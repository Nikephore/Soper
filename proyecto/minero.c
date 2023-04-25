#include "minero.h"

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

  /* Variables de memoria compartida */
  Memoria *mem;
  Bloque b_actual;
  Bloque b_ultimo;
  Cartera *carteras_mineros;

  /* Variables de vbloque */
  long target = 0, solution = -1;

  /* Variables de terminal */
  int n_seconds = 0, n_threads = 0;

  char *strptr;
  Dato resultado, fin;
  struct mq_attr attr;
  mqd_t queue;

  attr.mq_maxmsg = MAX_MSG;
  attr.mq_msgsize = sizeof(Dato);
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
  /* Inicializamos los atributos del bloque final*/
  fin.fin = true;
  fin.objetivo = -1;
  fin.solucion = -1;
  fin.correcto = false;

  /* Formateamos argumentos de la terminal */
  n_cycles = atoi(argv[1]);
  number_range_error_handler(1, MAX_SECONDS, n_seconds, "Number of seconds");

  n_threads = atoi(argv[2]);
  number_range_error_handler(1, MAX_THREADS, n_threads, "Number of threads");

  printf("[%d] Soy el proceso Minero...\n", getpid());

  minero_pid = fork();
	pid_error_handler(minero_pid);

  /* Es el proceso Registrador*/
  if(minero_pid == 0)
  {

  } 
  else /* Es el proceso Minero */
  {
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


        if (ftruncate(memoria, SHM_SIZE) == -1)
        {
            perror("ftruncate");
            close(memoria);
            exit(EXIT_FAILURE);
        }

        mem = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, memoria, 0);
        mem->mineros = NULL;
        mem->votos = NULL;
        mem->monedas = NULL;

        carteras_mineros = calloc(1, sizeof(Cartera))
        if (carteras_mineros == NULL)
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