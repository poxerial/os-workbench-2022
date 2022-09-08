#include <stdio.h>

#include <unistd.h>
#include <sys/wait.h>

extern char **environ;

int main()
{
    char * const args[] = {"gcc", "-fPIC", "-shared", "-m64", "-O1",
   "-std=gnu11", "-ggdb", "-Wall", "-Werror", "-Wno-unused-result",
    "-Wno-unused-value", "-Wno-unused-variable", "-o", "__1662645003_640704__wrapper", 
    "__1662645003_640704__wrapper.c", "-ldl", NULL};
    int error_code = execve("gcc", args, environ);
    printf("%d\n", error_code);
}