
/* Print a stacktrace for each memory allocation and free.
 *
 * Compile:
 * gcc -g -O0 -D_GNU_SOURCE -std=gnu99 -ldl -fPIC -shared -Wl,-soname,memlog.so memlog.c -o memlog.so
 *
 * Usage:
 * LD_PRELOAD=./memlog.so program
 */

#include <malloc.h>    /* __malloc_hook, __realloc_hook, __free_hook */
#include <execinfo.h>  /* backtrace(), backtrace_symbols()           */


#define STACKTRACE_LINES 20


static void* malloc_hook(size_t size, const void *caller);
static void* realloc_hook(void *ptr, size_t size, const void *caller);
static void  free_hook    (void *ptr, const void *caller);


static void* (*old_malloc_hook )(size_t, const void*       );
static void* (*old_realloc_hook)(void*, size_t, const void*);
static void  (*old_free_hook   )(void*, const void*        );


static void print_function(const char* f, void* p1, void* p2) {
  void*  array[STACKTRACE_LINES + 2];
  size_t size;

  size = backtrace(array, sizeof(array) / sizeof(array[0]));
  
  fprintf(stderr, "%s %p %p\n", f, p1, p2);
  backtrace_symbols_fd(&array[2], size - 2, 2);
  fprintf(stderr, "\n");
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


static void* malloc_hook(size_t size, const void *caller) {
  (void)caller;

  unhook();

  void* ptr = malloc(size);
  
  print_function("m", ptr, 0);

  hook();

  return ptr;
}


static void* realloc_hook(void *ptr, size_t size, const void *caller) {
  unhook();

  void* newptr = realloc(ptr, size);

  print_function("r", ptr, newptr);
  
  hook();

  return newptr;
}


static void free_hook(void *ptr, const void *caller) {
  unhook();

  print_function("f", ptr, 0);

  free(ptr);

  hook();
}


static void memdebug_init(void) {
  hook();
}

void (*__MALLOC_HOOK_VOLATILE __malloc_initialize_hook)(void) = memdebug_init;

