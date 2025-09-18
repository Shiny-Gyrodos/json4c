#include <stdlib.h>
#include <string.h>

#include "json_allocator.h"

static void* _std_alloc(ptrdiff_t, void*);
static void _std_free(void*, ptrdiff_t, void*);
static void* _std_realloc(void*, ptrdiff_t, ptrdiff_t, void*);
static void* _backupRealloc(void*, ptrdiff_t, ptrdiff_t, void*);


struct Allocator json_allocator = (struct Allocator){_std_alloc, _std_free, _std_realloc, NULL};


void json_allocator_set(
	void* (*custom_alloc)(ptrdiff_t, void*), 
	void (*custom_free)(void*, ptrdiff_t, void*),
	void* (*custom_realloc)(void*, ptrdiff_t, ptrdiff_t, void*),
	void* context
) {
	json_allocator = (struct Allocator){custom_alloc, custom_free, custom_realloc, context};
	if (!json_allocator.realloc) {
		json_allocator.realloc = _backupRealloc;
	}
}

void json_allocator_reset(void) {
	json_allocator = (struct Allocator){_std_alloc, _std_free, _std_realloc, NULL};
}


static void* _std_alloc(ptrdiff_t size, void* context) {
	(void)context;
	return malloc((size_t)size);
}

static void _std_free(void* ptr, ptrdiff_t size, void* context) {
	(void)size;
	(void)context;
	free(ptr);
}

static void* _std_realloc(void* ptr, ptrdiff_t newSize, ptrdiff_t oldSize, void* context) {
	(void)oldSize;
	(void)context;
	return realloc(ptr, (size_t)newSize);
}

static void* _backupRealloc(void* ptr, ptrdiff_t newSize, ptrdiff_t oldSize, void* context) {
	void* newptr = json_allocator.alloc(newSize, context);
	memcpy(newptr, ptr, oldSize);
	json_allocator.free(ptr, oldSize, context);
	return newptr;
}
