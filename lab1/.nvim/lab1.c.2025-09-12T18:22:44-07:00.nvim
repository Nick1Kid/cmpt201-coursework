#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {

  // From the getline man page.
  char *buff = NULL;
  size_t size = 0;
  ssize_t char_read;

  printf("Please enter some text: ");
  char_read = getline(&buff, &size, stdin);

  if (char_read == -1) {
    perror("Getline failed");
    exit(EXIT_FAILURE);
    free(buff);
  }

  char *saveptr;
  char *ret = strtok_r(buff, " ", &saveptr);

  printf("Tokens: \n");

  while (ret != NULL) {
    printf(" %s\n", ret);

    ret = strtok_r(NULL, " ", &saveptr);
  }

  free(buff);
}
