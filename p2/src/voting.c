#include "voting.h"

#define MAX_PROCS 100
#define MAX_SECS 60
#define MIN_SECS 1

#define SEM_CANDIDATE "/sem_candidate"

static volatile sig_atomic_t usr1 = 0;
static volatile sig_atomic_t usr2 = 0;

void catcher(int sig)
{
    switch (sig)
    {
    case SIGUSR1:
        usr1 = 1;
    case SIGUSR2:
        usr2 = 1;
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

void pid_error_handler(pid_t pid)
{
    if (pid < 0)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    return;
}

void voter()
{
    fprintf(stdout, "Im a voter, mi id is %d\n", getpid());

    fprintf(stdout, "%d | Antes del while \n", getpid());

    // sigsuspend(&set);

    fprintf(stdout, "%d | Return from suspended successful\n", getpid());

    return;
}

int main(int argc, char *argv[])
{
    int n_procs, n_secs;
    int written = 0;
    pid_t main_pid;
    pid_t *voter_id;
    FILE *fp;
    struct sigaction sact;
    sigset_t sigset;
    sem_t *sem_c = NULL;

    sem_unlink(SEM_CANDIDATE);

    if ((sem_c = sem_open(SEM_NAME_A, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0)) ==
        SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    /* Control de errores de argumentos */
    if (argc != 3)
    {
        fprintf(stderr, "Correct format: ./voting <N_PROCS> <N_SECS>\n");
        exit(EXIT_FAILURE);
    }

    n_procs = atoi(argv[1]);
    number_range_error_handler(1, MAX_PROCS, n_procs, "Number of processes");

    n_secs = atoi(argv[2]);
    number_range_error_handler(MIN_SECS, MAX_SECS, n_secs, "Number of seconds");

    voter_id = (pid_t *)calloc(n_procs, sizeof(pid_t));
    if (!voter_id)
    {
        fprintf(stderr, "Can't allocate child processes memory\n");
        exit(EXIT_FAILURE);
    }

    sigemptyset(&sact.sa_mask);
    sact.sa_flags = 0;
    sact.sa_handler = catcher;
    if (sigaction(SIGUSR1, &sact, NULL) != 0)
        perror("1st sigaction() error");

    main_pid = getpid();

    /* Creamos los N Procesos */
    for (int i = 0; i < n_procs; i++)
    {
        voter_id[i] = fork();
        pid_error_handler(voter_id[i]);

        /* Es un proceso hijo */
        if (voter_id[i] == 0)
        {

            // voter();

            printf("valor flag antes %d \n", usr1);

            sigdelset(&sigset, SIGUSR1);
            printf("%d is waiting for SIGUSR1\n", getpid());
            sigsuspend(&sigset);

            exit(EXIT_SUCCESS);
        }
    }

    if (main_pid == getpid())
    {
        fprintf(stdout, "Im the main process, mi id is %d\n", getpid());

        fp = fopen("pids.bin", "w+");
        if (fp == NULL)
        {
            perror("fopen");
            exit(EXIT_FAILURE);
        }

        written = fwrite(voter_id, sizeof(pid_t), n_procs, fp);
        if (written == 0)
        {
            for (int i = 0; i < n_procs; i++)
                wait(NULL);
            fprintf(stderr, "Error writing voters ids on file\n");
        }

        for (int i = 0; i < n_procs; i++)
        {
            fprintf(stdout, "Sending SIGUSR1 to %d\n", voter_id[i]);
            kill(voter_id[i], SIGUSR1);
        }

        fprintf(stdout, "%d | terminado kill\n", getpid());

        for (int i = 0; i < n_procs; i++)
        {
            wait(NULL);
            fprintf(stdout, "he hecho un wait\n");
        }

        fprintf(stdout, "%d | terminado waits\n", getpid());

        fclose(fp);
        free(voter_id);
        // free(return_ids);
    }

    /*
        fseek(fp, 0, SEEK_SET);
        red = fread(return_ids, sizeof(pid_t), n_procs, fp);
        fprintf(stdout, "Red: %d | Content of file:\n", red);
        for(int i = 0; i < n_procs; i++)
        {
            fprintf(stdout, "%d\n", return_ids[i]);
        }
        */

    /*
    if ((sem = sem_open(SEM_NAME, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0)) == SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }
    */
    exit(EXIT_SUCCESS);
}