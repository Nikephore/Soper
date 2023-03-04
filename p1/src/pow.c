#include "pow.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#define PRIME POW_LIMIT

#define BIG_X 435679812
#define BIG_Y 100001819

#define MAX_THREADS 100
#define MAX_CYCLES 20
#define ERROR -1
#define NOTFOUND 0
#define FOUND 1

int target_found = NOTFOUND; /* Estara a 0 si no se ha encontrado el target y a 1 cuando se encuentre*/

typedef struct
{
  int min;
  int max;
  int target;
  int solution;
} search;

long int pow_hash(long int x)
{
  long int result = (x * BIG_X + BIG_Y) % PRIME;
  return result;
}

void * target_search(void *objective)
{
  
  search *obj = objective;
  int result;

  int contador = 0;
  
  for(int i = obj->min; i <= obj->max; i++){
    contador++;

    if(target_found == FOUND){
      fprintf(stdout, "ALGUIEN HA ENCONTRADO LA SOLUCION Y NO HE SIDO YO, YO HE REVISADO %d DE %d NUMEROS POSIBLES\n", contador, obj->max-obj->min);
      pthread_exit((void*)obj);
    }

    result = pow_hash(i);
    if(result == obj->target){
      target_found = FOUND;
      fprintf(stdout, "El valor de target_found es -> %d, i = %d\n", target_found, i);
      result = i;
      obj->solution = result;
      pthread_exit((void*)obj);
    }
  }

  fprintf(stdout, "NO HE ENCONTRADO LA SOLUCION Y HE BUSCADO TODAS LAS POSIBILIDADES\n");
}

int solve(int n_threads, int target)
{
  int solution = -1;
  int error;
  pthread_t threads[n_threads];
  search objective[n_threads];

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

    if(objective[i].solution != -1){
      solution = objective[i].solution;
    }
    fprintf(stdout, "Objetivo: %d | valor de solucion: %d\t\t| Hilo numero %d\n", objective[i].target, objective[i].solution, i);
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

  fprintf(stdout, "El numero de hilos es: %d\n\n\n", n_threads);

  for (int i = 0; i < n_cycles; i++)
  {
    printf("\n\nEstamos en la ronda %d\n\n", i + 1);
    target_found = NOTFOUND;
    solution = solve(n_threads, target_ini);
    
    if(solution == -1){
      printf("No se ha encontrado una solucion para la ronda %d\n", i + 1);
    }
    target_ini = solution;
  }
}