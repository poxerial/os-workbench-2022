#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>



int main(int argc, char *argv[]) {

  // int pipedes[2];
  // assert(pipe(pipedes) == 0);

  // int pid = fork();

  // if (pid == 0) {

  // }

  char *exec_argv[] = { "strace", "ls", NULL, };
  char *exec_envp[] = { "PATH=/bin", NULL, };
  execve("strace",          argv, exec_envp);
  execve("/bin/strace",     argv, exec_envp);
  execve("/usr/bin/strace", argv, exec_envp);
  perror(argv[0]);
  exit(EXIT_FAILURE);
}
