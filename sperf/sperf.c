#include <alloca.h>
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

  char *exec_envp[] = { "PATH=/bin", NULL, };


  char **exec_argv= (char**)alloca(argc * sizeof(char *));
  for (int i = 0; i < argc; i++) {
    exec_argv[i] = argv[i];
  }
  exec_argv[argc] = "-r";

  execve("strace",          exec_argv, exec_envp);
  execve("/bin/strace",     exec_argv, exec_envp);
  execve("/usr/bin/strace", exec_argv, exec_envp);
  perror(argv[0]);
  exit(EXIT_FAILURE);
}
