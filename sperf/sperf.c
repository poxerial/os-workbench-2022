#include <alloca.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {

  char *exec_envp[] = {
      "PATH=/bin",
      NULL,
  };
  char **exec_argv = (char **)alloca((argc + 2) * sizeof(char *));
  exec_argv[0] = "strace";
  exec_argv[1] = "-r";
  for (int i = 1; i < argc; i++) {
    exec_argv[i + 1] = argv[i];
  }
  exec_argv[argc + 1] = NULL;

  execve("strace", exec_argv, exec_envp);
  execve("/bin/strace", exec_argv, exec_envp);
  execve("/usr/bin/strace", exec_argv, exec_envp);
  perror(argv[0]);
  exit(EXIT_FAILURE);

  // int pipedes[2];
  // assert(pipe(pipedes) == 0);
  // int pid = fork();
  // if (pid == 0) {
  //   execve("strace", exec_argv, exec_envp);
  //   execve("/bin/strace", exec_argv, exec_envp);
  //   execve("/usr/bin/strace", exec_argv, exec_envp);
  //   perror(argv[0]);
  //   exit(EXIT_FAILURE);
  // }
}
