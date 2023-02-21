#include <stdio.h>
#include <errno.h>

#define FILE1 "file1.txt"

int main(int argc, char *argv[]) {
    int error;
    
    if (!(fopen(argv[1], "r"))) {
        error = errno;
        perror("fopen");
        printf("errno: %d\n", error);
    }
}