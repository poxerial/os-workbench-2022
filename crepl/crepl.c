#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <unistd.h>
#include <sys/time.h>
#include <wait.h>
#include <dlfcn.h>

#define EXPR 0
#define FUNC 1

#ifdef __x86_64
#define MACHINE_OPTION "-m64"
#else
#define MACHINE_OPTION "-m32"
#endif


extern char **environ;

int check(const char *);
void gen_name(char *);
void add_func(const char *);
void eval_expr(const char *);
int compile(const char *, char *);
void* load(const char *name);
int execute(void *);


int main(int argc, char *argv[]) {
  static char line[4096];
  while (1) {
    printf("crepl> ");
    fflush(stdout);
    if (!fgets(line, sizeof(line), stdin)) {
      break;
    }
    if (check(line) == EXPR)
    {
      eval_expr(line);
    }
    else
    {
      add_func(line);
    }
  }
}

int check(const char *line)
{
  int result;
  if (strstr(line, "int"))
    result = FUNC;
  else
    result = EXPR;
  return result;
}

void gen_name(char *name)
{
  struct timeval time;
  gettimeofday(&time, NULL);
  sprintf(name, "__%ld_%ld__", time.tv_sec, time.tv_usec);
}

void add_func(const char *line)
{
  char compile_time_str[64];
  gen_name(compile_time_str);
  if (compile(line, compile_time_str))
  {
    printf("Compile failed.\n");
  }
  else 
  {
    load(compile_time_str);
    printf("Func added.\n");
  }
}

void eval_expr(const char *line)
{
  char *wrapper_content = (char *)malloc(strlen(line) + 128);
  char name[64];
  gen_name(name);
  strcat(name, "wrapper");
  sprintf(wrapper_content, 
  "int %s()"
  "{"
  "  return %s;"
  "}",
  name, line);

  if (compile(wrapper_content, name))
  {
    printf("Compile failed!\n");
  }
  else
  {
    void *handle = load(name);
    int result = execute(handle);
    printf("%d\n", result);
  }
}

int compile(const char *codes, char *name)
{
  char filename[64];
  strcpy(filename, name);
  FILE *source_code = fopen(strcat(filename, ".c"), "w");
  fprintf(source_code, "%s", codes);
  fclose(source_code);

  char * const args[] = {"gcc", "-fPIC", "-shared", MACHINE_OPTION, "-O1",
   "-std=gnu11", "-ggdb", "-Wall", "-Werror", "-Wno-unused-result",
    "-Wno-unused-value", "-Wno-unused-variable", "-o", name, filename, "-ldl", NULL};

  int pid = fork();
  if (pid == 0)
  {
    int error_code = execvpe("gcc", args, environ);
    exit(error_code);
  }
  int wait_status;
  wait(&wait_status);
  return wait_status; 
}

void* load(const char *name)
{
  return dlopen(name, RTLD_LAZY);
}

int execute(void *handle)
{
  Dl_info info;   
  dladdr(handle, &info);
  printf("fname: %s sname: %s\n", info.dli_fname, info.dli_sname);
  return 0;
}
