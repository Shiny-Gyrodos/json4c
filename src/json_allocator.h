#ifndef JSON4C_ALLOCATOR
#define JSON4C_ALLOCATOR

#include <stddef.h>

#include "json_config.h"

struct Allocator {
	void* (*alloc)(ptrdiff_t size, void* context);
	void (*free)(void* ptr, ptrdiff_t size, void* context);
	void* (*realloc)(void* ptr, ptrdiff_t newSize, ptrdiff_t oldSize, void* context);
	void* context;
};

// NOTE: The libraries' allocator is a global variable.
extern struct Allocator allocator;

// stdlib wrappers
void* json_std_alloc(ptrdiff_t, void*);
void json_std_free(void*, ptrdiff_t, void*);
void* json_std_realloc(void*, ptrdiff_t, ptrdiff_t, void*);

// Is used if json_realloc is set to NULL.
void* json_allocator_backupRealloc(void*, ptrdiff_t, ptrdiff_t, void*);

// NOTE: Only set/reset the allocator before nodes have been allocated or after they have all been freed.
void json_allocator_set(
	void* (*custom_alloc)(ptrdiff_t, void*), 
	void (*custom_free)(void*, ptrdiff_t, void*),
	void* (*custom_realloc)(void*, ptrdiff_t, ptrdiff_t, void*),
	void* context
);
void json_allocator_reset(void);

#endif // JSON4C_ALLOCATOR
