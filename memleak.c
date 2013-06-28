
/* Print leaked memory when exiting (including stacktrace).
 *
 * Compile:
 * gcc -g -O0 -D_GNU_SOURCE -std=gnu99 -ldl -fPIC -shared -Wl,-soname,memleak.so memleak.c -o memleak.so
 *
 * Usage:
 * LD_PRELOAD=./memleak.so program
 */

#include <string.h>    /* strdup()                                   */
#include <malloc.h>    /* __malloc_hook, __realloc_hook, __free_hook */
#include <execinfo.h>  /* backtrace(), backtrace_symbols()           */
#include <assert.h>    /* assert()                                   */
#include <stdlib.h>    /* atexit()                                   */

#include "tree.h"


typedef struct allocation_s {
  RB_ENTRY(allocation_s) node;

  size_t size;

  void*  trace[10];
  size_t trace_size;
} allocation_t;

static int   compare_allocations(const allocation_t* a, const allocation_t* b);
static void* malloc_hook (size_t size, const void *caller);
static void* realloc_hook(void *ptr, size_t size, const void *caller);
static void  free_hook   (void *ptr, const void *caller);

typedef struct allocation_tree_s {
  allocation_t* rbh_root;
} allocation_tree_t;

RB_GENERATE_STATIC(allocation_tree_s, allocation_s, node, compare_allocations)

static allocation_tree_t allocation_tree;

static void* (*old_malloc_hook )(size_t, const void*       );
static void* (*old_realloc_hook)(void*, size_t, const void*);
static void  (*old_free_hook   )(void*, const void*        );


static int compare_allocations(const allocation_t* a, const allocation_t* b) {
  if (a == b) {
    return 0;
  }
  return (a < b) ? 1 : -1;
}


static void get_trace(allocation_t* f) {
  f->trace_size = backtrace(f->trace, sizeof(f->trace) / sizeof(f->trace[0]));
}


static void hook() {
  old_realloc_hook = __realloc_hook;
  __realloc_hook   = realloc_hook;

  old_malloc_hook = __malloc_hook;
  __malloc_hook   = malloc_hook;

  old_free_hook = __free_hook;
  __free_hook   = free_hook;
}


static void unhook() {
  __realloc_hook = old_realloc_hook;
  __malloc_hook  = old_malloc_hook;
  __free_hook    = old_free_hook;
}


static void print_all() {
  unhook();

  printf("\n\nmemleak:\n");
 
  for (allocation_t* a = RB_MIN(allocation_tree_s, &allocation_tree); a != 0; a = RB_NEXT(allocation_tree_s, &allocation_tree, a)) {
    printf("%p (%zu):\n", a, a->size);
    
    char** symbols = backtrace_symbols(a->trace, a->trace_size);

    for (size_t s = 2; s < a->trace_size; ++s) {
      printf(" %s\n", symbols[s]);
    }

    printf("\n");
  }
}


static void* malloc_hook(size_t size, const void *caller) {
  (void)caller;

  unhook();

  allocation_t* a = malloc(size + sizeof(allocation_t));
  a->size         = size;
  get_trace(a);

  assert(RB_INSERT(allocation_tree_s, &allocation_tree, a) == 0);

  hook();

  return ((void*)a) + sizeof(allocation_t);
}


static void* realloc_hook(void *ptr, size_t size, const void *caller) {
  (void)caller;

  unhook();

  allocation_t* a;

  if (ptr != 0) {
    a = RB_FIND(allocation_tree_s, &allocation_tree, (allocation_t*)(ptr - sizeof(allocation_t)));

    assert(a);

    assert(RB_REMOVE(allocation_tree_s, &allocation_tree, a) != 0);

    a = realloc(a, size + sizeof(allocation_t));
  } else {
    a = malloc(size + sizeof(allocation_t));
  }

  a->size = size;
  get_trace(a);

  assert(RB_INSERT(allocation_tree_s, &allocation_tree, a) == 0);

  hook();

  return ((void*)a) + sizeof(allocation_t);
}


static void free_hook(void *ptr, const void *caller) {
  (void)caller;

  unhook();

  if (ptr) {
    allocation_t* a = RB_FIND(allocation_tree_s, &allocation_tree, (allocation_t*)(ptr - sizeof(allocation_t)));

    assert(a);

    assert(RB_REMOVE(allocation_tree_s, &allocation_tree, a) != 0);

    free(a);
  }

  hook();
}


static void memdebug_init(void) {
  RB_INIT(&allocation_tree);

  assert(atexit(print_all) == 0);

  hook();
}

void (*__MALLOC_HOOK_VOLATILE __malloc_initialize_hook)(void) = memdebug_init;

