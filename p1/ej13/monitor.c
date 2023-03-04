#include "monitor.h"

void monitor()
{
    fprintf(stdout, "Soy el proceso monitor y mi pid es %d\n", getpid());
    return;
}

