#ifndef JSON4C_UTILS
#define JSON4C_UTILS

#include <stdbool.h>
#include <stdio.h>
#include <stddef.h>

#define CONSTRAIN(condition)

#define json_utils_ensureCapacity(ptr, capacity, count)			\
	json_utils_ensureCapacity_impl((void**)ptr, sizeof(*(*(ptr))), capacity, count)
void json_utils_ensureCapacity_impl(void**, size_t, ptrdiff_t*, ptrdiff_t);

#define json_utils_dynAppendStr(bufferptr, lengthptr, offsetptr, ...)	\
	json_utils_dynAppendStr_impl(bufferptr, lengthptr, offsetptr, (char*[]){__VA_ARGS__, NULL})
void json_utils_dynAppendStr_impl(char**, ptrdiff_t*, ptrdiff_t*, char**);

bool json_buf_expect(char, char*, ptrdiff_t, ptrdiff_t*);
char json_buf_get(char*, ptrdiff_t, ptrdiff_t*);
char json_buf_unget(char, char*, ptrdiff_t, ptrdiff_t*);
char json_buf_peek(char*, ptrdiff_t, ptrdiff_t);

#endif
