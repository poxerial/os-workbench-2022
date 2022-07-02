#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <assert.h>

#include "co.h"

#define STACK_SIZE (1 << 16)

enum co_status
{
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

static jmp_buf local_buf;
static struct co *top;
static struct co *tobedel;
static struct co *current;
static size_t co_num;

static inline void stack_switch_call(void *sp, void *entry, uintptr_t arg);
static void randjmp();
void entry(void *args);
struct co *co_start(const char *_name, void (*_func)(void *), void *_arg);
void co_wait(struct co *co);
void co_yield ();
int co_free(struct co *);

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

int co_free(struct co *co)
{
  struct co *c;
  int is_deleted = 0;
  if (tobedel->next == NULL || tobedel == co)
  {
    assert(tobedel == co);
    tobedel = tobedel->next;
    is_deleted = 1;
  }
  else
  {
    for (c = tobedel; c->next != NULL; c = c->next)
    {
      if (c->next == co)
      {
        c->next = c->next->next;
        is_deleted = 1;
        break;
      }
    }
  }
  assert(is_deleted);
  free((void *)co);
  return is_deleted;
}

static void randjmp()
{
  if (co_num == 0)
    return;
  int r = rand();
  struct co *ne = (struct co *)top;
  for (r %= co_num; r > 0; r--)
  {
    ne = ne->next;
  }
  assert(ne != NULL);

  current = ne;
  if (ne->status == CO_NEW)
  {
    ne->status = CO_HAS_RUN;
    stack_switch_call(ne->stack + STACK_SIZE - 8, entry, (uintptr_t)ne);
  }
  else
  {
    longjmp(ne->context, 1);
  }
}

void entry(void *args)
{
  struct co *c = (struct co *)args;
  c->func(c->arg);

  int is_move_to_tobedel = 0;
  current->status = CO_HAS_RUN;
  if (top == current)
  {
    top = top->next;
    current->next = tobedel;
    tobedel = current;
    is_move_to_tobedel = 1;
  }
  else
  {
    for (c = top; c->next != NULL; c = c->next)
    {
      if (c->next == current)
      {
        c->next = c->next->next;
        current->next = tobedel;
        tobedel = current;
        is_move_to_tobedel = 1;
        break;
      }
    }
  }
  assert(is_move_to_tobedel);
  current = NULL;

  co_num--;

  longjmp(local_buf, 1);
}

struct co *co_start(const char *_name, void (*_func)(void *), void *_arg)
{
  struct co *c = (struct co *)malloc(sizeof(struct co));
  assert(c);
  co_num++;
  c->name = _name;
  c->func = _func;
  c->arg = _arg;
  c->status = CO_NEW;
  if (top)
    c->next = (struct co *)top;
  else
    c->next = NULL;
  top = c;
  return c;
}

void co_wait(struct co *co)
{
  struct co *c;

  for (c = tobedel; c; c = c->next)
  {
    if (c == co)
    {
      co_free(co);
      return;
    }
  }

  if (setjmp(local_buf) == 0)
  {
    randjmp();
  }
  
  co_wait(co);
}

void co_yield ()
{
  if (setjmp(current->context) == 0)
    randjmp();
}
