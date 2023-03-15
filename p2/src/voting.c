#include "voting.h"

#define MAX_PROCS   100
#define MAX_SECS    60
#define MIN_SECS    10

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
}

int main(int argc, char *argv[]){
    int n_procs, n_secs;
    int status;
    int written = 0, red = 0;
    pid_t main_pid;
    pid_t * voter_id;
    pid_t * return_ids;
    FILE * fp;
    void * buffer;

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
    if(!voter_id)
    {
        fprintf(stderr, "Can't allocate child processes memory\n");
        exit(EXIT_FAILURE);
    }

    return_ids = (pid_t *)calloc(n_procs, sizeof(pid_t));
    if(!return_ids)
    {
        fprintf(stderr, "Can't allocate child processes memory\n");
        exit(EXIT_FAILURE);
    }

    main_pid = getpid();

    for(int i = 0; i < n_procs; i++)   
    {
        voter_id[i] = fork();
        pid_error_handler(voter_id[i]);

        if(voter_id[i] == 0)
        {
            
            voter();
            exit(EXIT_SUCCESS);
        }
        else{
            
            waitpid(voter_id[i], &status, 0);

        }
    }

    if(main_pid == getpid())
    {
        fprintf(stdout, "Im the main process, mi id is %d\n", getpid());
        for(int i = 0; i < n_procs; i++)
        {
            fprintf(stdout, "%d\n", voter_id[i]);
        }

        fp = fopen("test.txt", "w");
        written = fwrite(voter_id, sizeof(pid_t), n_procs, fp);
        fprintf(fp, voter_id);
        if(written == 0)
        {  
            for(int i = 0; i < n_procs; i++) wait(NULL);
            fprintf(stderr, "Error writing voters ids on file\n");
        }

        fseek(fp, 0, SEEK_SET);

        red = fread(return_ids, sizeof(pid_t), n_procs, fp);
        fprintf(stdout, "Red: %d | Content of file:\n", red);
        for(int i = 0; i < n_procs; i++)
        {
            fprintf(stdout, "%d\n", return_ids[i]);
        }



        fclose(fp);
        free(voter_id);
        free(return_ids);

    }

    /*
    if ((sem = sem_open(SEM_NAME, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0)) == SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }
    */



    
}