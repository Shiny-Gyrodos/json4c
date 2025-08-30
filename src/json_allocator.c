#include <stdlib.h>

#include "json_allocator.h"

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
