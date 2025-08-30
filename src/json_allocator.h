#ifndef JSON4C_ALLOCATOR
#define JSON4C_ALLOCATOR

#include <stddef.h>

typedef struct {
	void* (*json_alloc)(ptrdiff_t size, void* context);
	void (*json_free)(void* ptr, ptrdiff_t size, void* context);
	void* (*json_realloc)(void* ptr, ptrdiff_t newSize, ptrdiff_t oldSize, void* context);
	void* context;
} Allocator;

// Usage: Allocator allocator = ALLOCATOR_DEFAULT;
#define ALLOCATOR_DEFAULT (Allocator){json_std_alloc, json_std_free, json_std_realloc, NULL}

// stdlib wrappers
void* json_std_alloc(ptrdiff_t, void*);
void json_std_free(void*, ptrdiff_t, void*);
void* json_std_realloc(void*, ptrdiff_t, ptrdiff_t, void*);

#endif // JSON4C_ALLOCATOR
