#ifndef CDH_MEMORY_H
#define CDH_MEMORY_H

/*
Wrappers for malloc, realloc, and free that call custom implementations
to provide useful debug capabilities. Should be included last to properly
overwrite all references to malloc.
*/

#ifdef DEBUG

#include <stdlib.h>
#define malloc(n) cdh_debug_malloc(n)
#define realloc(ptr, n) cdh_debug_realloc(ptr, n)
#define calloc(n, size) cdh_debug_calloc(n, size)
#define free(ptr) cdh_debug_free(ptr)

#endif // DEBUG

/*
If you would like to define your own memory functions that will override
the std library ones, define this macro. Otherwise the following overrides
will be used in debug mode.
*/
#ifndef CDH_USE_CUSTOM_MEMORY_FUNCTIONS

#include <stddef.h>
void* cdh_debug_malloc(const size_t numOfBytes, const char* file, const int line);
void* cdh_debug_realloc(void* heapPointer, const size_t numOfBytes, const char* file, const int line);
void* cdh_debug_calloc(const size_t elementCount, const size_t sizeOfElement, const char* file, const int line);
void cdh_debug_free(void* heapPointer, const char* file, const int line);
#define cdh_debug_malloc(n) cdh_debug_malloc(n, __FILE__, __LINE__)
#define cdh_debug_realloc(ptr, n) cdh_debug_realloc(ptr, n, __FILE__, __LINE__)
#define cdh_debug_calloc(n, size) cdh_debug_calloc(n, size, __FILE__, __LINE__)
#define cdh_debug_free(ptr) cdh_debug_free(ptr, __FILE__, __LINE__)

#endif // CDH_USE_CUSTOM_MEMORY_FUNCTIONS

#endif // CDH_MEMORY_H