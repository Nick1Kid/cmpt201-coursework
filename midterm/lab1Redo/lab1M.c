#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
int main() {
  char *buffer = NULL;
  size_t size = 0;

  printf("Please enter some text: ");

  ssize_t reading = getline(&buffer, &size, stdin);

  if (reading == -1) {
    perror("getline failed\n");
    exit(EXIT_FAILURE);
    free(buffer);
  }

  char *saveptr;
  char *ret = strtok_r(buffer, " ", &saveptr);

  printf("Tokens: \n");

  while (ret != NULL) {
    printf(" %s\n", ret);

    ret = strtok_r(NULL, " ", &saveptr);
  }

  free(buffer);
}
