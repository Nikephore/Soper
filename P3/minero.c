#include <mqueue.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "pow.h"

#define MQ_NAME "/mq_example"
#define MAX_MESSAGES 7

typedef struct {
    bool fin;
    long objetivo;
    long solucion;
    bool correcto;
} Dato;

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
  long result=0;

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
  long target=0, solution=-1;
  int n_cycles = 0, lag=0;
  Dato resultado=NULL, fin=NULL;
  struct mq_attr attr;
  mqd_t queue;


  attributes.mq_maxmsg = MAX_MESSAGES;
  attributes.mq_msgsize = sizeof(Dato);

  queue = mq_open(QUEUE_NAME, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, &attr);
  if (queue == -1)
  {
    if (errno == EEXIST)
    {
      queue = mq_open(QUEUE_NAME, O_RDWR, 0);
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
  else
  {
    printf("Message queue created\n");
  }


  fin.fin = TRUE;
  fin.objetivo = -1;
  fin.solucion = -1;
  fin.correcto = FALSE;


  n_cycles = atoi(argv[1]);
  number_range_error_handler(1, MAX_CYCLES, n_cycles, "Number of cycles");

  lag = atoi(argv[2]);


  fprintf(stdout, "[%d] Generating blocks...\n", getpid());


  for (int i = 0; i < n_cycles; i++)
  {
    solution = target_search(target);

    if (solution == -1)
    {
      printf("Error on miner process\n");
      return EXIT_FAILURE;
    }

    resultado.fin = FALSE;
    resultado.objetivo = target;
    resultado.solucion = solution;
    resultado.correcto = TRUE;

    if (mq_send(queue, (Dato)resultado, sizeof(resultado), 0) == -1) {
      perror("mq_send");
      mq_close(queue);
      exit(EXIT_FAILURE);
    }
    
    target = solution;

    usleep(lag*1000);
  }

  /* All rounds finished, notifying the monitor */
  solution = -1;
  if (mq_send(queue, (Dato)fin, sizeof(fin), 0) == -1) {
    perror("mq_send");
    mq_close(queue);
    exit(EXIT_FAILURE);
  }

  mq_close(queue);

  fprintf(stdout, "Finishing\n");

  return EXIT_SUCCESS;
}