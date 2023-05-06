#include "common.h"

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