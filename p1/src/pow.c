#include "pow.h"

#define PRIME POW_LIMIT
#define BIG_X 435679812
#define BIG_Y 100001819

#define MAX_THREADS 100
#define MAX_CYCLES 20
#define ERROR -1

long int pow_hash(long int x) {
  long int result = (x * BIG_X + BIG_Y) % PRIME;
  return result;
}

*void target_search(rango definido){
  
  //bucle que busque en pow hash desde s_ini a s_fin
  //comprobar variable global
  

}

int main(int argc, char **argv){

    int n_threads;
    int n_cycles;

    //array de 0 a POW_LIMIT-1

    //igualar variables a argv[]

    if(n_cycles > 0){
      for(int a = 0; a < n_cycles;  a++){
        for(int i = 0; i < n_threads; i++){
          //crear hilo

        }
        for(int i = 0; i < n_threads; i++){
          //join hilos
        }

        
      }
    }
}

/*
main(n_t, n_c, target_inicial){

  for(n_c){
    sol = solve(nthreads, tini)
    tini = sol
  }
}
*/

int main(int argc, char *argv[]){

  int n_cycles;
  int n_threads;
  int target_ini;
  int solution;

  /* Control de errores num arguentos*/
  if(argc != 4) {
    printf("No se ha pasado el numero correcto de argumentos\n");
    printf("El formato correcto es:\n");
    printf("./programa <Target inicial> <Num_ciclos> <Num_hilos>");
    return ERROR;
  }

  target_ini = atoi(argv[1]);
  if(target_ini < 0){
    printf("El target no puede ser inferior a 0");
    return ERROR;
  }

  /* Establecemos el numero de ciclos */
  n_cycles = atoi(argv[2]);
  if(n_threads < 1){
    printf("El numero de ciclos no puede ser inferior a 1");
    return ERROR;
  }

  if(n_threads > MAX_CYCLES){
    printf("El numero de hilos no puede ser superior a %d", MAX_CYCLES);
    return ERROR;
  }

  /* Establecemos el numero de hilos */
  n_threads = atoi(argv[3]);
  if(n_threads < 1){
    printf("El numero de hilos no puede ser inferior a 1");
    return ERROR;
  }

  if(n_threads > MAX_THREADS){
    printf("El numero de hilos no puede ser superior a %d", MAX_THREADS);
    return ERROR;
  }

  for(i = 0; i < n_cycles; i++){
    printf("Estamos en el bucle %d", i+1);
    solution = solve(n_threads, target_ini);
    target_ini = solution;
  }

}

int solve(int n_threads, int target){
  int solution = -1;
  pthread_t threads[nthreads];

  for(i = 0; i < n_threads; i++){
    
  }



  return solution
}

/*
solve(n_t, t){
  por cada hilo
    create thread -> cada uno en un rango
  por cada hilo
    join thread
}
*/