#include "voting.h"

#define MAX_PROCS 100
#define MAX_SECS 60
#define MIN_SECS 1

#define SEM_CANDIDATE "/sem_candidate"
#define SEM_FILE "/sem_file"

static volatile sig_atomic_t usr1 = 0;
static volatile sig_atomic_t usr2 = 0;
static volatile sig_atomic_t sint = 0;
static volatile sig_atomic_t salarm = 0;

void catcher(int sig)
{
    switch (sig)
    {
    case SIGUSR1:
        usr1 = 1;
        break;
    case SIGUSR2:
        usr2 = 1;
        break;
    case SIGINT:
        sint = 1;
    case SIGALRM:
        salarm = 1;
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

void cleaner(voter *voters, FILE *fd)
{
    free(voters);
    fclose(fd);

    return;
}

void voter_process(struct sigaction sact, sem_t *sem_c, sem_t *sem_f, int n_procs)
{
    int candidate = 0, result = 0;
    FILE *fp;
    voter aux_voter;

    printf("%d nprocs\n", n_procs);

    sigfillset(&sact.sa_mask);
    sigdelset(&sact.sa_mask, SIGTERM);
    sigdelset(&sact.sa_mask, SIGUSR1);
    do
    {
        printf("%d is waiting for SIGUSR1\n", getpid());
        sigsuspend(&sact.sa_mask);

        /* Bloqueamos SIGUSR1 */
        sigaddset(&sact.sa_mask, SIGUSR1);

        /* Desbloqueamos SIGUSR2 para poder hacer la votacion*/
        sigdelset(&sact.sa_mask, SIGUSR2);

        printf("%d is trying to be the candidate\n", getpid());
        /* Intenta ser el proceso candidato, si sem_trywait es 0 lo consigue*/
        if (sem_trywait(sem_c) == 0)
        {
            candidate = 1;
            printf("YO, %d SOY EL PROCESO CANDIDATO\n", getpid());
            /* Enviar SIGUSR2 a los procesos para comenzar la votacion*/
            fp = fopen("pids.bin", "r+b");
            if (fp == NULL)
            {
                perror("fopen");
                exit(EXIT_FAILURE);
            }
            for (int i = 0; i < n_procs; i++)
            {

                while (fread(&aux_voter, sizeof(voter), 1, fp))
                {
                    printf("%d valor leido del fichero\n", aux_voter.voter_pid);
                    /* Leemos los pids de los procesos desde el fichero para enviar SIGUSR2 */
                    if (aux_voter.voter_pid != getpid())
                    {
                        printf("%d no es el candidato\n", aux_voter.voter_pid);
                        fprintf(stdout, "Sending SIGUSR2 to %d | %d i\n", aux_voter.voter_pid, aux_voter.vote);
                        kill(aux_voter.voter_pid, SIGUSR2);
                    }
                }
            }
            fclose(fp);
        }
        else
        {
            printf("YO, %d waiting for SIGUSR2\n", getpid());
            sigsuspend(&sact.sa_mask);
        }

        printf("YO, %d SOY UN VOTANTE\n", getpid());
        /* Esperando a recibir SIGUSR2 para poder votar */

        /* Comienzo de la zona critica, votante registrando el voto*/
        sem_wait(sem_f);
        printf("%d ha entrado en el semaforo\n", getpid());
        /* Abrimos fichero */
        fp = fopen("pids.bin", "r+b");
        if (fp == NULL)
        {
            perror("fopen");
            exit(EXIT_FAILURE);
        }

        while (fread(&aux_voter, sizeof(voter), 1, fp) == 1)
        {
            /* Buscamos el elemento asociado a este proceso para introducir el voto */
            if (aux_voter.voter_pid == getpid())
            {
                aux_voter.vote = rand() % 2;
                fseek(fp, ftell(fp) - sizeof(voter), SEEK_SET);
                fwrite(&aux_voter, sizeof(voter), 1, fp);
                fclose(fp);
            }
        }
        sem_post(sem_f);

        if (candidate == 1)
        {

            sleep(5);
            fp = fopen("pids.bin", "r+b");
            if (fp == NULL)
            {
                perror("fopen");
                exit(EXIT_FAILURE);
            }

            printf("Candidate %d => [ ", getpid());
            while (fread(&aux_voter, sizeof(voter), 1, fp) == 1)
            {
                /* Leemos los votos que han dejado los votantes */
                if (aux_voter.vote == 0)
                    printf("N ");
                else if (aux_voter.vote == 1)
                {
                    result++;
                    printf("Y ");
                }

                /* Establecemos voto a valor por defecto para la siguiente ronda*/
                aux_voter.vote = -1;
                fseek(fp, ftell(fp) - sizeof(voter), SEEK_SET);
                fwrite(&aux_voter, sizeof(voter), 1, fp);
            }
            fclose(fp);
            /* Segun el resultado obtenido imprimimos aceptado o rechazado*/
            if (result > n_procs / 2)
                printf("] => Accepted\n");
            else
                printf("] => Rejected\n");

            /* Espera no activa de 0.25 segundos antes de la siguiente ronda*/
            sleep(0.25);

            /* Enviar SIGUSR1 a los procesos para comenzar siguiente ronda*/
            for (int i = 0; i < n_procs; i++)
            {
                fp = fopen("pids.bin", "r+b");
                if (fp == NULL)
                {
                    perror("fopen");
                    exit(EXIT_FAILURE);
                }

                while (fread(&aux_voter, sizeof(voter), 1, fp) == 1)
                {
                    fprintf(stdout, "Sending SIGUSR1 to %d\n", aux_voter.voter_pid);
                    kill(aux_voter.voter_pid, SIGUSR1);
                }
                fclose(fp);
            }
        }

        /* Desbloqueamos SIGUSR1 para esperar a la siguiente ronda */
        sigaddset(&sact.sa_mask, SIGUSR1);

        /* Volvemos al comienzo del bucle para esperar a nueva ronda */

    } while (1);

    return;
}

// Pasar como parametro de la funcion main a otra funcion secundaria un array de una estructura con 2 campos, uno de tipo pid_t y otro int. Esta estructura se podra editar desde la funcion secundaria. Cada elemento del array sera editado por un proceso distinto creado a traves de fork en la funcion main. La actualizacion del array debe verse reflejada en el resto de procesos hijos

int main(int argc, char *argv[])
{
    int n_procs, n_secs;
    int written = 0;
    pid_t main_pid;
    pid_t child_id;
    voter *voters;
    FILE *fp;
    struct sigaction sact;
    sigset_t sigset;
    sem_t *sem_c = NULL, *sem_f = NULL;

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

    /* En n_secs se mandara SIGALRM */
    alarm(n_secs);

    /* Inicializar semaforo de eleccion de candidato a 1 */
    sem_unlink(SEM_CANDIDATE);
    sem_unlink(SEM_FILE);

    if ((sem_c = sem_open(SEM_CANDIDATE, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1)) == SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    if ((sem_f = sem_open(SEM_FILE, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1)) == SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    voters = (voter *)calloc(n_procs, sizeof(voter));
    if (!voters)
    {
        fprintf(stderr, "Can't allocate child processes memory\n");
        exit(EXIT_FAILURE);
    }

    /* Inicializacion de señales */
    sigemptyset(&sact.sa_mask);
    sact.sa_flags = 0;
    sact.sa_handler = catcher;
    if (sigaction(SIGUSR1, &sact, NULL) != 0)
        perror("sigaction() SIGUSR1 error");

    if (sigaction(SIGUSR2, &sact, NULL) != 0)
        perror("sigaction() SIGUSR1 error");

    if (sigaction(SIGINT, &sact, NULL) != 0)
        perror("sigaction() SIGUSR1 error");

    if (sigaction(SIGALRM, &sact, NULL) != 0)
        perror("sigaction() SIGUSR1 error");

    main_pid = getpid();

    /* Creamos los N Procesos */
    for (int i = 0; i < n_procs; i++)
    {

        child_id = fork();
        pid_error_handler(child_id);

        /* Es un proceso hijo */
        if (child_id == 0)
        {
            voter_process(sact, sem_c, sem_f, n_procs);
            exit(EXIT_SUCCESS);
        }

        /* Guardamos los datos de la estructura */
        voters[i].voter_pid = child_id;
        voters[i].vote = -1;
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
        sleep(1);
        for (int i = 0; i < n_procs; i++)
        {
            fprintf(stdout, "Proceso hijo %d es %d\n", i, voters[i].voter_pid);
            written = fwrite(&voters[i], sizeof(voter), 1, fp);
            if (written == 0)
            {
                for (int i = 0; i < n_procs; i++)
                {
                    kill(voters[i].voter_pid, SIGTERM);
                    wait(NULL);
                }
                fprintf(stderr, "Error writing voters ids on file\n");
                free(voters);
                fclose(fp);
                exit(EXIT_FAILURE);
            }
        }
        fclose(fp);

        /* Enviar SIGUSR1 a los procesos para comenzar la primera ronda*/
        for (int i = 0; i < n_procs; i++)
        {
            fprintf(stdout, "Sending SIGUSR1 to %d\n", voters[i].voter_pid);
            kill(voters[i].voter_pid, SIGUSR1);
        }

        do
        {
            sleep(1);
        } while (sint == 0 && salarm == 0);

        for (int i = 0; i < n_procs; i++)
        {
            fprintf(stdout, "Sending SIGTERM to %d\n", voters[i].voter_pid);
            kill(voters[i].voter_pid, SIGTERM);
            wait(NULL);
            fprintf(stdout, "he hecho un wait\n");
        }

        sem_close(sem_c);
        sem_unlink(SEM_CANDIDATE);

        if (sint == 1)
        {
            fprintf(stdout, "Finishing by signal\n");
        }
        else
        {
            fprintf(stdout, "Finishing by alarm\n");
        }

        free(voters);
        sem_close(sem_c);
        sem_close(sem_f);
        sem_unlink(SEM_CANDIDATE);
        sem_unlink(SEM_FILE);
    }
    else
    {
    }

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

*/