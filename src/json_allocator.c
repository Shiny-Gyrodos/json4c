#include <stdlib.h>

#include "json_allocator.h"

struct Allocator allocator = JSON_ALLOCATOR_DEFAULT;


void* json_std_alloc(ptrdiff_t size, void* context) {
	(void)context;
	return malloc((size_t)size);
}

void json_std_free(void* ptr, ptrdiff_t size, void* context) {
	(void)size;
	(void)context;
	free(ptr);
}

void json_std_realloc(void* ptr, ptrdiff_t newSize, ptrdiff_t oldSize, void* context) {
	(void)oldSize;
	(void)context;
	return realloc(ptr, (size_t)newSize);
}


void json_allocator_set(
	void* (*json_alloc)(ptrdiff_t, void*), 
	void (*json_free)(void*, ptrdiff_t, void*),
	void* (*json_realloc)(void*, ptrdiff_t, ptrdiff_t, void*)
) {
	allocator = (struct Allocator){json_alloc, json_free, json_realloc, context};
}

void json_allocator_reset(void) {
	allocator = JSON_ALLOCATOR_DEFAULT;
}
