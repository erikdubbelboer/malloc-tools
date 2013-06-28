
Shared objects that can be used to debug memory allocations.

These objects need to be loaded using LD\_PRELOAD and will hook all calls to `malloc()`, `realloc()` and `free()`.

