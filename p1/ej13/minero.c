#include "minero.h"

#define ERROR -1
#define NOTFOUND 0
#define FOUND 1

int target_found = NOTFOUND; /* Estara a 0 si no se ha encontrado el target y a 1 cuando se encuentre*/

/**
 * @brief Search the correct value of the pow function between a specified range.
 *
 * @param objective Pointer to structure with the range values and the target to search.
 * @return Exit of the thread.
 */
void *target_search(void *objective)
{

  search *obj = objective;
  int result;

  for (int i = obj->min; i <= obj->max; i++)
  {

    if (target_found == FOUND)
    {
      pthread_exit((void *)obj);
    }

    result = pow_hash(i);
    if (result == obj->target)
    {
      target_found = FOUND;
      obj->solution = i;
      pthread_exit((void *)obj);
    }
  }

  pthread_exit((void *)obj);
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

int miner(int n_cycles, int n_threads, int target, int miner_to_monitor, int monitor_to_miner)
{
  ssize_t nbytes;
  int solution;
  int solution_check = 1; /* 0 if solution is correct, 1 if not */

  for (int i = 0; i < n_cycles; i++)
  {
    solution = manage_threads(n_threads, target);

    if (solution == -1)
    {
      printf("Error on miner process\n");
      return EXIT_FAILURE;
    }

    // Test to check invalidated solutions
    // if (i == 5) solution = 10;

    /* Send the solution to Monitor to verify it */
    nbytes = write(miner_to_monitor, &solution, sizeof(solution));
    if (nbytes == -1)
    {
      perror("write");
      return EXIT_FAILURE;
    }

    /* Recieve the Monitor solution check */
    nbytes = read(monitor_to_miner, &solution_check, sizeof(solution_check));
    if (nbytes == -1)
    {
      perror("read");
      return EXIT_FAILURE;
    }

    /* The solution has been invalidated */
    if(solution_check == 1)
    {
      fprintf(stdout, "The solution has been invalidated\n");
      solution = -1;
      write(miner_to_monitor, &solution, sizeof(int));
      return EXIT_FAILURE;
    }
    
    target = solution;
  }

  /* All rounds finished, notifying the monitor */
  solution = -1;
  nbytes = write(miner_to_monitor, &solution, sizeof(int));
  if (nbytes == -1)
    {
      perror("write");
      return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}