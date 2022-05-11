#include "co.h"
#include <inttypes.h>
#include <stdlib.h>
#include <setjmp.h>

#define STACK_SIZE 1 << 23

enum co_status{
  CO_NEW=1,
  CO_RUNNING,
  CO_WAITING,
  CO_DEAD,
};

struct co{
  //ensure 16-byte aligned 
  uint8_t stack[STACK_SIZE]; 

  char *name;
  void (*func)(void *);
  void *arg;

  enum co_status status;
  struct co *next;
  jmp_buf context;
};

struct co* current;

struct co *co_start(const char *name, void (*func)(void *), void *arg) {
  struct co* c = (struct co*) malloc(sizeof(struct co));
  if (c == NULL)
    return NULL;
  c->name = name;
  c->func = func;
  c->arg = arg;
  c->status = CO_NEW;
  
}

void co_wait(struct co *co) {
}

void co_yield() {
}
