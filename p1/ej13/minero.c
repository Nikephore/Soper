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

  int contador = 0;

  for (int i = obj->min; i <= obj->max; i++)
  {
    contador++;

    if (target_found == FOUND)
    {
      // fprintf(stdout, "ALGUIEN HA ENCONTRADO LA SOLUCION Y NO HE SIDO YO, YO HE REVISADO %d DE %d NUMEROS POSIBLES\n", contador, obj->max - obj->min);
      pthread_exit((void *)obj);
    }

    result = pow_hash(i);
    if (result == obj->target)
    {
      target_found = FOUND;
      // fprintf(stdout, "El valor de target_found es -> %d, i = %d\n", target_found, i);
      obj->solution = i;
      pthread_exit((void *)obj);
    }
  }

  fprintf(stdout, "NO HE ENCONTRADO LA SOLUCION Y HE BUSCADO TODAS LAS POSIBILIDADES\n");
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
    exit(EXIT_FAILURE);
  }

  for (int i = 0; i < n_threads; i++)
  {

    objective[i].min = POW_LIMIT / n_threads * i;
    objective[i].max = (POW_LIMIT / n_threads * (i + 1)) - 1;
    objective[i].target = target;
    objective[i].solution = -1;
    error = pthread_create(&threads[i], NULL, target_search, &objective[i]);
    if (error != 0)
    {
      fprintf(stderr, "pthread_create: %s\n", strerror(error));
      exit(EXIT_FAILURE);
    }
  }

  for (int i = 0; i < n_threads; i++)
  {
    error = pthread_join(threads[i], NULL);
    if (error != 0)
    {
      fprintf(stderr, "pthread_join: %s\n", strerror(error));
      exit(EXIT_FAILURE);
    }

    if (objective[i].solution != -1)
    {
      solution = objective[i].solution;
    }
    fprintf(stdout, "Objetivo: %ld | valor de solucion: %d\t\t| Hilo numero %d\n", objective[i].target, objective[i].solution, i);
  }

  free(threads);

  return solution;
}

int miner(int n_cycles, int n_threads, int target)
{
  int solution;

  for (int i = 0; i < n_cycles; i++)
  {
    printf("\n\nEstamos en la ronda %d\n\n", i + 1);
    solution = manage_threads(n_threads, target);

    if (solution == -1)
    {
      printf("No se ha encontrado una solucion para la ronda %d\n", i + 1);
      exit(EXIT_FAILURE);
    }
    target = solution;
  }

  return EXIT_SUCCESS;
}