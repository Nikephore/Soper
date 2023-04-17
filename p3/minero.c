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
  long target = 0, solution = -1;
  int n_cycles = 0, lag = 0;
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
    printf("./miner <ROUNDS> <LAG>\n");
    printf("El lag se medira en milisegundos, maximo 10000\n");

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
  number_range_error_handler(1, MAX_CYCLES, n_cycles, "Number of cycles");

  lag = (unsigned int)strtoul(argv[1], &strptr, 10);
  if (*strptr != '\0')
  {
    printf("Valor inválido: '%s' no es un numero\n", strptr);
    exit(EXIT_FAILURE);
  }
  number_range_error_handler(MIN_LAG, MAX_LAG, lag, "Lag");

  printf("[%d] Generating blocks...\n", getpid());

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