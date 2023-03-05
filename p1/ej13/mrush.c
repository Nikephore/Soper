#include "minero.h"
#include "monitor.h"

#define MAX_THREADS 100
#define MAX_CYCLES 20

void pid_error_handler(pid_t pid)
{
	if (pid < 0)
	{
		perror("fork");
		exit(EXIT_FAILURE);
	}
	return;
}

void pipe_error_handler(int pipe_status)
{
	if (pipe_status == -1)
	{
		perror("pipe");
		exit(EXIT_FAILURE);
	}
}

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

int main(int argc, char *argv[])
{
	pid_t miner_pid, monitor_pid;
	int n_cycles, n_threads, target_ini;

	/* Define the pipes */
	int miner_to_monitor[2];
	int monitor_to_miner[2];

	int pipe_status;

	/* Control de errores num arguentos*/
	if (argc != 4)
	{
		printf("No se ha pasado el numero correcto de argumentos\n");
		printf("El formato correcto es:\n");
		printf("./programa <Target inicial> <Num_ciclos> <Num_hilos>");
		exit(EXIT_FAILURE);
	}

	target_ini = atoi(argv[1]);
	number_range_error_handler(0, POW_LIMIT - 1, target_ini, "Target");

	/* Establecemos el numero de ciclos */
	n_cycles = atoi(argv[2]);
	number_range_error_handler(1, MAX_CYCLES, n_cycles, "Number of cycles");

	/* Establecemos el numero de hilos */
	n_threads = atoi(argv[3]);
	number_range_error_handler(1, MAX_THREADS, n_threads, "Number of threads");

	fprintf(stdout, "Soy el proceso principal, mi pid es %d\n", getpid());

	/* [0] is to Read | [1] is to Write */
	pipe_status = pipe(miner_to_monitor);
	pipe_error_handler(pipe_status);

	pipe_status = pipe(monitor_to_miner);
	pipe_error_handler(pipe_status);

	miner_pid = fork();
	pid_error_handler(miner_pid);

	if (miner_pid == 0)
	{
		monitor_pid = fork();
		pid_error_handler(monitor_pid);

		if (monitor_pid == 0) /* Monitor process */
		{
			/* Close the read end on the monitor */
			close(monitor_to_miner[0]);
			/* Close the write end on the monitor */
			close(miner_to_monitor[1]);

			fprintf(stdout, "Im Monitor, my id is: %d | My parent is %d\n", getpid(), getppid());
			monitor(monitor_to_miner[1], miner_to_monitor[0]);
		}
		else /* Miner process */
		{
			/* Close the read end on the miner */
			close(miner_to_monitor[0]);
			/* Close the write end on the miner */
			close(monitor_to_miner[1]);

			miner(n_cycles, n_threads, target_ini, miner_to_monitor[1], monitor_to_miner[0]);
			wait(NULL);
			printf("Monitor exited with status %d\n", WIFEXITED(monitor_pid));
		}
	}
	else
	{
		wait(NULL);
		printf("Miner exited with status %d\n", WIFEXITED(miner_pid));
	}

	exit(EXIT_SUCCESS);
}