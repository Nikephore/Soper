#include "minero.h"
#include "monitor.h"

#define MAX_THREADS 100
#define MAX_CYCLES 20
#define ERROR -1

void pid_error_handler(pid_t pid)
{
	if (pid < 0)
	{
		perror("fork");
		exit(EXIT_FAILURE);
	}
	return;
}

int main(int argc, char *argv[])
{
	pid_t pid;
	int n_cycles;
	int n_threads;
	int target_ini;
	int ret;

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

	fprintf(stdout, "Soy el proceso principal, mi pid es %d\n", getpid());

	pid = fork();
	pid_error_handler(pid);

	if (pid == 0)
	{
		fprintf(stdout, "Im Miner, my id is: %d | My parent is %d\n", getpid(), getppid());
		pid = fork();
		pid_error_handler(pid);

		if(pid == 0){
			fprintf(stdout, "Im Monitor, my id is: %d | My parent is %d\n", getpid(), getppid());
			monitor();
		} 
		else
		{
			ret = miner(n_cycles, n_threads, target_ini);
		}
		wait(NULL);
	}
	wait(NULL);

	return ret;
}