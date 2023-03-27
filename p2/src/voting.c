#include "voting.h"

#define MAX_PROCS 100
#define MAX_SECS 60
#define MIN_SECS 1

#define SEM_CANDIDATE "/sem_candidate"
#define SEM_FILE "/sem_file"
#define SEM_SIGNAL "/sem_signal"

static volatile sig_atomic_t sint = 0;
static volatile sig_atomic_t salarm = 0;

void catcher(int sig)
{
    switch (sig)
    {
    case SIGUSR1:
        break;
    case SIGUSR2:
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

void voter_process(struct sigaction sact, sem_t *sem_c, sem_t *sem_f, sem_t *sem_s, int n_procs)
{
    int candidate = 0, result = 0, vote;
    //int bytes = 0;
    FILE *fp;
    voter aux_voter;

    sigfillset(&sact.sa_mask);
    sigdelset(&sact.sa_mask, SIGTERM);
    sigdelset(&sact.sa_mask, SIGUSR1);

    do
    {
        /* Si el proceso no es el candidato de la ronda anterior
            espera a recibir SIGUSR1 por parte del candidato
            o por parte del proceso principal (primera ronda)*/
        if (candidate == 0)
        {
            printf("%d is waiting for SIGUSR1\n", getpid());
            sem_post(sem_s);
            sigsuspend(&sact.sa_mask);
            printf("%d waited for SIGUSR1\n", getpid());
        }

        /* Bloqueamos SIGUSR1 */
        sigaddset(&sact.sa_mask, SIGUSR1);

        /* Desbloqueamos SIGUSR2 para poder hacer la votacion*/
        sigdelset(&sact.sa_mask, SIGUSR2);

        /* Intenta ser el proceso candidato, si sem_trywait es 0 lo consigue*/
        printf("%d INTENTA SER CANDIDATO\n", getpid());
        if (sem_trywait(sem_c) == 0)
        {

            candidate = 1;
            printf("YO, %d SOY EL PROCESO CANDIDATO\n", getpid());

            /* Dejamos el fichero de los votos vacio */
            fp = fopen("votes.bin", "w");
            if (fp == NULL)
            {
                perror("fopen");
                exit(EXIT_FAILURE);
            }
            fclose(fp);

            /* Enviar SIGUSR2 a los procesos para comenzar la votacion*/

            // inicializa un semaforo usando la funcion sem_open(). El valor inicial del semaforo sera 1

            for (int i = 0; i < n_procs - 1; i++)
            {
                sem_getvalue(sem_s, &result);
                printf("\n\n%d EL VALOR DEL SEMAFORO ES %d ANTES DEL WAIT\n\n", getpid(), result);
                sem_wait(sem_s);
                sem_getvalue(sem_s, &result);
                printf("\n\n%d EL VALOR DEL SEMAFORO ES %d \n EL CANDIDATO HA HECHO %d WAITS\n\n", getpid(), result, i + 1);
            }

            printf("-------------\nCOMPROBAR QUE TODOS ESTAN ESPERANDO A SIGUSR2 %d\n------------------\n", getpid());
            kill(0, SIGUSR2);
        }
        else /* Procesos que no son candidatos */
        {
            sem_getvalue(sem_s, &result);
            printf("\n\n%d EL VALOR DEL SEMAFORO ANTES ES %d \n", getpid(), result);
            sem_post(sem_s);
            sem_getvalue(sem_s, &result);
            printf("\n\n%d EL VALOR DEL SEMAFORO DESPUES ES %d \n", getpid(), result);
            printf("---- %d WAITING FOR SIGUSR2 ----\n", getpid());
            sigsuspend(&sact.sa_mask);
            printf("%d recieved SIGUSR2\n", getpid());
        }

        printf("YO, %d SOY UN VOTANTE\n", getpid());
        /* Esperando a recibir SIGUSR2 para poder votar */

        /* Comienzo de la zona critica, votante registrando el voto*/
        sem_wait(sem_f);
        printf("%d ha entrado en el semaforo\n", getpid());
        /* Abrimos fichero */
        fp = fopen("votes.bin", "a+");
        if (fp == NULL)
        {
            perror("fopen");
            exit(EXIT_FAILURE);
        }

        // Calcular valor aleatorio 0 o 1 e introducirlo en una variable int
        srand(time(NULL));

        vote = (pow_hash(getpid())) % 2;
        fwrite(&vote, sizeof(int), 1, fp);
        fclose(fp);
        sem_post(sem_f);
        /* Fin de la zona critica */

        if (candidate == 1)
        {
            printf("\nEL CANDIDATO YA HA VOTADO\n");
            fp = fopen("votes.bin", "r");
            if (fp == NULL)
            {
                perror("fopen");
                exit(EXIT_FAILURE);
            }

            /*
            do
            {
                fseek(fp, 0, SEEK_SET);
                sleep(0.005);
                printf("*");
                bytes = fread(&bytes, sizeof(int), n_procs, fp);
                printf("%d", bytes);
            } while (bytes != n_procs);
            */

            sleep(5);
            // printf("\n\n%ld\n\n", fread(&bytes, sizeof(int), n_procs, fp));
            // printf("\nVOY A LEER LOS VOTOS\n");
            // printf("Candidate %d => [ ", getpid());
            while (fread(&vote, sizeof(int), 1, fp))
            {
                /* Leemos los votos que han dejado los votantes */
                if (vote == 0)
                    printf("N ");
                else if (vote == 1)
                {
                    result++;
                    printf("Y ");
                }
            }
            fclose(fp);
            /* Segun el resultado obtenido imprimimos aceptado o rechazado*/
            if (result > n_procs / 2)
                printf("] => Accepted\n");
            else
                printf("] => Rejected\n");

            /* Espera no activa de 0.25 segundos antes de la siguiente ronda*/
            sleep(0.25);

            for (int i = 0; i < n_procs - 1; i++)
            {
                sem_wait(sem_s);
                sem_getvalue(sem_s, &result);
            }
            /* Enviar SIGUSR1 a los procesos para comenzar siguiente ronda*/
            fp = fopen("pids.bin", "r+b");
            if (fp == NULL)
            {
                perror("fopen");
                exit(EXIT_FAILURE);
            }

            while (fread(&aux_voter, sizeof(voter), 1, fp) == 1)
            {
                if (aux_voter.voter_pid != getpid())
                {
                    fprintf(stdout, "Sending SIGUSR1 to %d\n", aux_voter.voter_pid);
                    kill(aux_voter.voter_pid, SIGUSR1);
                }
            }
            fclose(fp);

            sem_post(sem_c);
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
    sem_t *sem_c = NULL, *sem_f = NULL, *sem_s = NULL;

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

    if ((sem_s = sem_open(SEM_SIGNAL, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0)) == SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    /* Inicializar semaforo de eleccion de candidato a 1 */
    sem_unlink(SEM_CANDIDATE);
    sem_unlink(SEM_FILE);
    sem_unlink(SEM_SIGNAL);

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
            voter_process(sact, sem_c, sem_f, sem_s, n_procs);
            exit(EXIT_SUCCESS);
        }
        else /* Si es el proceso padre */
        {
            /* Guardamos los datos de la estructura */
            voters[i].voter_pid = child_id;
            voters[i].vote = -1;
        }
    }

    if (main_pid == getpid())
    {
        fprintf(stdout, "Im the main process, mi id is %d\n", getpid());

        /* Ponemos a 0 el semaforo de fichero para escribir la estructura de los votantes */
        sem_wait(sem_f);
        fp = fopen("pids.bin", "w+");
        if (fp == NULL)
        {
            perror("fopen");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < n_procs; i++)
        {
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
        /* Ponemos a 1 el semaforo de fichero */
        sem_post(sem_f);

        /* Hacemos tantos sem_waits como procesos haya
           para asegurarnos que todos esten en el sigsuspend
           antes de mandar SIGUSR1 */
        for (int i = 0; i < n_procs; i++)
            sem_wait(sem_s);

        /* Enviar SIGUSR1 a los procesos para comenzar la primera ronda*/
        fprintf(stdout, "Sending SIGUSR1 to child processes\n");
        kill(0, SIGUSR1);

        do
        {
            sleep(0.25);
        } while (sint == 0 && salarm == 0);

        for (int i = 0; i < n_procs; i++)
        {
            fprintf(stdout, "Sending SIGTERM to %d\n", voters[i].voter_pid);
            kill(voters[i].voter_pid, SIGTERM);
            wait(NULL);
        }

        if (sint == 1)
            fprintf(stdout, "Finishing by signal\n");
        else
            fprintf(stdout, "Finishing by alarm\n");

        free(voters);
        sem_close(sem_c);
        sem_close(sem_f);
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