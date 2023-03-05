#include "monitor.h"

void monitor(int monitor_to_miner, int miner_to_monitor)
{
    ssize_t nbytes = 0;
    int solution;
    fprintf(stdout, "Soy el proceso monitor y mi pid es %d\n", getpid());

    do
    {
        
        nbytes = read(miner_to_monitor, &solution, sizeof(solution));
        if (nbytes == -1)
        {
            perror("write");
            exit(EXIT_FAILURE);
        }

        fprintf(stdout, "\n\nTEST, nbytes -> %ld | solution: %d \n\n", nbytes, solution);

        /* All rounds completed */
        if(solution == -1) nbytes = 0;

        
    } while (nbytes != 0);

    fprintf(stdout, "\n\n\nTODAS LAS RONDAS SE HAN TERMINADO\n\n\n");

    exit(EXIT_SUCCESS);
}
