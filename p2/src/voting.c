#include "voting.h"

#define MAX_PROCS 100
#define MAX_SECS 60
#define MIN_SECS 10

static volatile sig_atomic_t usr1 = 0;
static volatile sig_atomic_t usr2 = 0;

void sigusr1_handler(int sig) { usr1 = 1; }

void sigusr2_handler(int sig) { usr2 = 1; }

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

    while (1)
    {
        printf("Waiting Ctrl+C (PID = %d)\n", getpid());
        if (usr1)
        {
            usr1 = 0;
            printf("Signal received.\n");
        }
        sleep(9999);
    }
}

int main(int argc, char *argv[])
{
    int n_procs, n_secs;
    int status;
    int written = 0, red = 0;
    pid_t main_pid;
    pid_t *voter_id;
    pid_t *return_ids;
    FILE *fp;
    struct sigaction act;

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

    /*
    return_ids = (pid_t *)calloc(n_procs, sizeof(pid_t));
    if(!return_ids)
    {
        fprintf(stderr, "Can't allocate child processes memory\n");
        exit(EXIT_FAILURE);
    }
    */

    main_pid = getpid();

    for (int i = 0; i < n_procs; i++)
    {
        voter_id[i] = fork();
        pid_error_handler(voter_id[i]);
    }

    if (main_pid == getpid())
    {
        fprintf(stdout, "Im the main process, mi id is %d\n", getpid());
        for (int i = 0; i < n_procs; i++)
        {
            fprintf(stdout, "%d\n", voter_id[i]);
        }

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

        act.sa_handler = sigusr1_handler;
        if (sigaction(SIGINT, &act, NULL) < 0)
        {
            perror("sigaction");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < n_procs; i++)
            kill(voter_id, SIGUSR1);

        for (int i = 0; i < n_procs; i++)
            wait(NULL);

        fclose(fp);
        free(voter_id);
        // free(return_ids);
    }
    else
    {
        voter();
        exit(EXIT_SUCCESS);
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
}