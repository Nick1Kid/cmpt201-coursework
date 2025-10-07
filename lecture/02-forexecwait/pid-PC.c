#include <stdio.h>
#include <unistd.h>

int main() {

  printf("Start PID=%d, parent PID=%d\n", getpid(), getppid());
  pid_t pid = fork();

  if (pid == 0) {
    printf("CHILD: PID=%d, parent PID=%d\n", getpid());

    else printf("PARENT: PID=%d, child PID=%d\n", getppid());
  }
}
