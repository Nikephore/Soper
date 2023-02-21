#include <stdio.h>
#include <time.h>
#include <unistd.h>

int main(int argc, char *argv[]) {

    printf("Active\n");
    while(clock() / CLOCKS_PER_SEC < 10);
    printf("Inactive\n");
    sleep(10);
    printf("End\n");

    return 0;
}