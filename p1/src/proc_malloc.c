#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MESSAGE "Hello"

int main(void) {
  pid_t pid;
  char *sentence = calloc(sizeof(MESSAGE), 1);

  printf("El mensaje es: %s\n", MESSAGE);

  pid = fork();
  printf("Mi pid antes de entrar al if es: %d\n", pid);
  if (pid < 0) {
    perror("fork");
    exit(EXIT_FAILURE);
  } else if (pid == 0) {
    strcpy(sentence, MESSAGE);
    printf("Child: %s\n", sentence);
    free(sentence);
    exit(EXIT_SUCCESS);
  } else {
    wait(NULL);
    printf("Parent: %s\n", sentence);
    free(sentence);
    exit(EXIT_SUCCESS);
  }
}
