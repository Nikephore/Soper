#include "monitor.h"

void monitor(int monitor_to_miner, int miner_to_monitor, int target)
{
    ssize_t nbytes = 0;
    int solution, solution_check;

    do
    {
        nbytes = read(miner_to_monitor, &solution, sizeof(solution));
        if (nbytes == -1)
        {
            perror("read");
            exit(EXIT_FAILURE);
        }

        if (solution != -1)
        {
            /* Checks the solution */
            if (target == pow_hash(solution))
            {
                solution_check = 0;
                fprintf(stdout, "Solution accepted: %08d --> %08d\n", target, solution);
                target = solution;
            }  
            else
            {
                solution_check = 1;
                nbytes = 0;
                fprintf(stdout, "Solution rejected: %08d !-> %08d\n", target, solution);
     
            }
            
            nbytes = write(monitor_to_miner, &solution_check, sizeof(solution_check));
            if (nbytes == -1)
            {
                perror("write");
                exit(EXIT_FAILURE);
            }
        }
        else nbytes = 0; /* All rounds completed */

    } while (nbytes != 0);

    exit(EXIT_SUCCESS);
}
