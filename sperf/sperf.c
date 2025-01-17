#include <assert.h>
#include <bits/pthreadtypes.h>
#include <bits/types/timer_t.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <syscall.h>
#include <unistd.h>
#include <pthread.h>
#include <regex.h>

#define BUFFER_SIZE 128
#define SYSCALL_MAX_NUM 100
#define SYSCALL_NAME_SIZE 16

#define ARRAY_SIZE(ARRAY) (sizeof(ARRAY) / sizeof(ARRAY[0]))

typedef struct {
  char name[SYSCALL_NAME_SIZE];
  double time;
} _syscall;

typedef struct {
  int num;

  _syscall s[SYSCALL_MAX_NUM];
} syscalls;

static const char regex[] = "^(\\w+)\\(.*<([0-9]+\\.[0-9]+)>$";
static syscalls syscs;

void new_syscall(syscalls *s, const char *const name, const double time) {
  for (int i = 0; i < s->num; i++) {
    if (0 == strcmp(s->s[i].name, name)) {
      s->s[i].time += time;
      return;
    }
  }
  if (s->num < SYSCALL_MAX_NUM) {
    assert(strlen(name) <= SYSCALL_NAME_SIZE);
    strcpy(s->s[s->num].name, name);
    s->s[s->num].time = time;
    s->num++;
  }
}

int _syscall_cmp(const void *a, const void *b) {
  return ((const _syscall *)a)->time < ((const _syscall *)b)->time;
}

void _print(syscalls *s) {
  qsort(s->s, s->num, sizeof(_syscall), _syscall_cmp);
  for (int i = 0; i < s->num; i++) {
    printf("%s %lf\n", s->s[i].name, s->s[i].time);
  }
}

void *wait_and_close_pipe(void *args) {
  int pid = ((int *)args)[0], des = ((int *)args)[1];
  waitpid(pid, NULL, 0);
  close(des);
  free(args);
  return NULL;
}

void set_wait(int pid, int des) {
  int *args = malloc(sizeof(int) * 2);
  args[0] = pid;
  args[1] = des;
  pthread_t id;
  pthread_attr_t attr;
  assert(pthread_attr_init(&attr) == 0);
  pthread_create(&id, &attr, wait_and_close_pipe, args);
}

int main(int argc, char *argv[]) {
  int pipedes[2];
  assert(pipe(pipedes) == 0);
  int pid = fork();
  if (pid == 0) {
    int null_des = open("/dev/null", O_WRONLY);

    dup2(null_des, STDOUT_FILENO);
    dup2(pipedes[1], STDERR_FILENO);

    char *exec_envp[] = {
        "PATH=/bin",
        NULL,
    };
    char **exec_argv = (char **)alloca((argc + 2) * sizeof(char *));
    exec_argv[0] = "strace";
    exec_argv[1] = "-T";
    for (int i = 1; i < argc; i++) {
      exec_argv[i + 1] = argv[i];
    }
    exec_argv[argc + 1] = NULL;

    execve("strace", exec_argv, exec_envp);
    execve("/bin/strace", exec_argv, exec_envp);
    execve("/usr/bin/strace", exec_argv, exec_envp);
    perror(argv[0]);
    exit(EXIT_FAILURE);
  } else {
    set_wait(pid, pipedes[1]);

    int n = 0;

    char buffer[BUFFER_SIZE] = {0};
    size_t buffer_offset = 0;

    time_t start;
    time(&start);

    regex_t preg;
    int errcode;
    if ((errcode = regcomp(&preg, regex, REG_EXTENDED))) {
      exit(errcode);
    }

    int size;
    while ((size = read(pipedes[0], buffer + buffer_offset,
                        BUFFER_SIZE - buffer_offset))) {
      char *end;
      if ((end = strchr(buffer, '\n')) == NULL) {
        buffer_offset += size;
        continue;
      }

      *end = '\0';

      regmatch_t matches[3];
      if ((errcode = regexec(&preg, buffer, 3, matches, 0))) {
        if (errcode != REG_NOMATCH) {
          char errbuf[512];
          regerror(errcode, &preg, errbuf, sizeof(errbuf));
          printf("regexec failed: %s.\n", errbuf);
          exit(errcode);
        }
      } else {

        buffer[matches[1].rm_eo] = '\0';
        buffer[matches[2].rm_eo] = '\0';

        new_syscall(&syscs, buffer + matches[1].rm_so,
                    atof(buffer + matches[2].rm_so));

        time_t now;
        time(&now);
        if (difftime(now, start) >= 1.0) {
          printf("==================");
          printf("time: %d s.", n++);
          printf("==================\n");
          _print(&syscs);
          syscs.num = 0;
          start = now;
        }
      }

      if (end - buffer + 1 != buffer_offset + size) {
        char tmp[BUFFER_SIZE];
        strcpy(tmp, end + 1);
        strcpy(buffer, tmp);
        buffer_offset = strlen(buffer);
      } else {
        buffer_offset = 0;
      }
    }
    if (syscs.num > 0) {
      _print(&syscs);
    }
  }
}
