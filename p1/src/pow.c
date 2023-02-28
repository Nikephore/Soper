#include "pow.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#define PRIME POW_LIMIT
#define BIG_X 435679812
#define BIG_Y 100001819

#define MAX_THREADS 100
#define MAX_CYCLES 20
#define ERROR -1

typedef struct
{
  int min;
  int max;
  int target;
} search;

long int pow_hash(long int x)
{
  long int result = (x * BIG_X + BIG_Y) % PRIME;
  return result;
}

void * target_search(void *objective)
{
  const search *obj = objective;
  int result;

  for(int i = obj->min; i <= obj->max; i++){
    result = pow_hash(i);
    if(result == obj->target){
      
    }
  }

  // bucle que busque en pow hash desde s_ini a s_fin
  // comprobar variable global
}

int solve(int n_threads, int target)
{
  int solution = -1;
  int error;
  pthread_t threads[n_threads];
  search objective[n_threads];

  // MAX / MAX_THREADS * i,  MAX / MAX_THREADS * (i+1) -1

  // 10000 / 100 = 100 * 0  --> 100

  for (int i = 0; i < n_threads; i++)
  {

    objective[i].min = POW_LIMIT / MAX_THREADS * i;
    objective[i].max = (POW_LIMIT / MAX_THREADS * (i + 1)) - 1;
    objective[i].target = target;
    error = pthread_create(&threads[i], NULL, target_search, objective);
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
  }

  return solution;
}

int main(int argc, char *argv[])
{

  int n_cycles;
  int n_threads;
  int target_ini;
  int solution;

  /* Control de errores num arguentos*/
  if (argc != 4)
  {
    printf("No se ha pasado el numero correcto de argumentos\n");
    printf("El formato correcto es:\n");
    printf("./programa <Target inicial> <Num_ciclos> <Num_hilos>");
    return ERROR;
  }

  target_ini = atoi(argv[1]);
  if (target_ini < 0)
  {
    printf("El target no puede ser inferior a 0");
    return ERROR;
  }

  /* Establecemos el numero de ciclos */
  n_cycles = atoi(argv[2]);
  if (n_cycles < 1)
  {
    printf("El numero de ciclos no puede ser inferior a 1");
    return ERROR;
  }

  if (n_cycles > MAX_CYCLES)
  {
    printf("El numero de ciclos no puede ser superior a %d", MAX_CYCLES);
    return ERROR;
  }

  /* Establecemos el numero de hilos */
  n_threads = atoi(argv[3]);
  if (n_threads < 1)
  {
    printf("El numero de hilos no puede ser inferior a 1");
    return ERROR;
  }

  if (n_threads > MAX_THREADS)
  {
    printf("El numero de hilos no puede ser superior a %d", MAX_THREADS);
    return ERROR;
  }

  for (int i = 0; i < n_cycles; i++)
  {
    printf("Estamos en el bucle %d", i + 1);
    solution = solve(n_threads, target_ini);
    target_ini = solution;
  }
}