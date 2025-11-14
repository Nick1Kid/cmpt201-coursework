#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int main() {

  char *buffer = NULL;
  size_t size = 0;

  while (1) {
    printf("Enter programs to run: ");
    printf("> ");
    fflush(stdout);

    ssize_t read = getline(&buffer, &size, stdin);

    if (buffer[read - 1] == '\n') {
      buffer[read - 1] == '\0';
    }

    pid_t pid = fork();

    if (pid == 0) {
      execl(buffer, buffer, (char *)NULL);

      perror("EXEC FAILED");
      exit(EXIT_FAILURE);
    } else {
      int stat;

      waitpid(pid, &stat, 0);
    }
  }

  free(buffer);
}
