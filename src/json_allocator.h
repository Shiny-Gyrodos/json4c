#ifndef JSON4C_ALLOCATOR
#define JSON4C_ALLOCATOR

#include <stddef.h>

struct Allocator {
	void* (*json_alloc)(ptrdiff_t size, void* context);
	void (*json_free)(void* ptr, ptrdiff_t size, void* context);
	void* (*json_realloc)(void* ptr, ptrdiff_t newSize, ptrdiff_t oldSize, void* context);
	void* context;
};

#ifndef JSON_ALLOCATOR_DEFAULT
#define JSON_ALLOCATOR_DEFAULT (struct Allocator){json_std_alloc, json_std_free, json_std_realloc, NULL}
#endif

// NOTE: The libraries' allocator is a global variable.
extern struct Allocator allocator;

// stdlib wrappers
void* json_std_alloc(ptrdiff_t, void*);
void json_std_free(void*, ptrdiff_t, void*);
void* json_std_realloc(void*, ptrdiff_t, ptrdiff_t, void*);

// NOTE: Only set/reset the allocator before nodes have been allocated or after they have all been freed.
void json_allocator_set(
	void* (*json_alloc)(ptrdiff_t, void*), 
	void (*json_free)(void*, ptrdiff_t, void*),
	void* (*json_realloc)(void*, ptrdiff_t, ptrdiff_t, void*),
	void* context
);
void json_allocator_reset(void);

#endif // JSON4C_ALLOCATOR
