#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define NUM_PROC 3

int main(void) {
  int i;
  pid_t pid;

  for (i = 0; i < NUM_PROC; i++) {
    pid = fork();
    if (pid < 0) {
      perror("fork");
      exit(EXIT_FAILURE);
    } else if (pid == 0) {
      printf("Child %d %d %d\n", i, getpid(), getppid());
      exit(EXIT_SUCCESS);
    } else if (pid > 0) {
      printf("Parent %d %d %d\n", i, getpid(), getppid());
    }
  }
  wait(NULL);
  exit(EXIT_SUCCESS);
}

/*
int main(void) {
  int i;
  pid_t pid;

  for (i = 0; i < NUM_PROC; i++) {
    pid = fork();
    if (pid < 0) {
      perror("fork");
      exit(EXIT_FAILURE);
    } else if (pid == 0) {
      printf("Child %d %d\n", getpid(), getppid());
    } else if (pid > 0) {
      printf("Parent %d %d\n", getpid(), getppid());
      exit(EXIT_SUCCESS);
    }
  }
  wait(NULL);
  exit(EXIT_SUCCESS);
}
*/