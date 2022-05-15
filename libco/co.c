#include "co.h"
#include <inttypes.h>
#include <stdlib.h>
#include <setjmp.h>

#define STACK_SIZE 1 << 16

enum co_status {
  CO_NEW = 1,
  CO_HAS_RUN,
};

struct co
{
  // ensure 16-byte aligned
  uint8_t stack[STACK_SIZE];

  const char *name;
  void (*func)(void *);
  void *arg;

  enum co_status status;
  struct co *next;
  jmp_buf context;
};

volatile struct co *top;
volatile struct co *current;
volatile size_t co_num = 0;
volatile size_t wait_num = 0;

static inline void stack_switch_call(void *sp, void *entry, uintptr_t arg)
{
  asm volatile(
#if __x86_64__
      "movq %0, %%rsp; movq %2, %%rdi; jmp *%1"
      :
      : "b"((uintptr_t)sp), "d"(entry), "a"(arg)
      : "memory"
#else
      "movl %0, %%esp; movl %2, 4(%0); jmp *%1"
      :
      : "b"((uintptr_t)sp - 8), "d"(entry), "a"(arg)
      : "memory"
#endif
  );
}

static void randjmp()
{
  if (co_num == 0)
    return;
  int r = rand();
  struct co *ne = (struct co*)top;
  for (r %= wait_num; r > 0; r--)
  {
    ne = ne->next;
  }
  
  current = ne;
  if (ne->status == CO_NEW)
  {
    ne->status = CO_HAS_RUN;
    stack_switch_call(ne->stack, ne->func, (uintptr_t)ne->arg);
  }
  else
  {
    longjmp(ne->context, 1);
  }
}

struct co *co_start(const char *_name, void (*_func)(void *), void *_arg)
{
  struct co *c = (struct co *)malloc(sizeof(struct co));
  if (c == NULL)
    return NULL;
  co_num++;
  c->name = _name;
  c->func = _func;
  c->arg = _arg;
  c->status = CO_NEW;
  if (top)
    c->next = (struct co*) top;
  else
    c->next = NULL;
  top = c;
  return c;
}

void co_wait(struct co *co)
{
  wait_num++;

  if (co_num == wait_num)
  {
    current = top;
    top->status = CO_HAS_RUN;
    stack_switch_call((void *)top->stack, top->func, (uintptr_t)top->arg);
    wait_num--;
    free((void *)current);
    current = NULL;
    if (wait_num)
    {
      randjmp();
    }
  }
}

void co_yield()
{
  if(setjmp(((struct co*)current)->context) == 0)
  {

  }
  else
  {

  }

  
  randjmp();
}
