#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int main() {
  char *line = NULL;
  size_t len = 0;
  ssize_t read;

  while (1) {
    printf("Enter programs to run.\n");
    printf("> ");
    fflush(stdout);

    read = getline(&line, &len, stdin);

    // Getline produces a trailing '\n', we must replace with '\0' (man page)
    if (line[read - 1] == '\n') {
      line[read - 1] = '\0';
    }

    pid_t pid = fork();

    if (pid == 0) {
      execl(line, line, (char *)NULL);

      perror("Exec failure");
      exit(EXIT_FAILURE);

    } else {
      int stat;
      waitpid(pid, &stat, 0);
    }
  }

  free(line);
}
