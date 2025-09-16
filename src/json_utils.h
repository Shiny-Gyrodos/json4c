#ifndef JSON4C_UTILS
#define JSON4C_UTILS

#include <stdbool.h>

bool json_buf_expect(char, char*, ptrdiff_t, ptrdiff_t*);
char json_buf_get(char*, ptrdiff_t, ptrdiff_t*);
char json_buf_put(char, char*, ptrdiff_t, ptrdiff_t*);
void json_buf_putstr(char*, char*, ptrdiff_t, ptrdiff_t*);
char json_buf_peek(char*, ptrdiff_t, ptrdiff_t);

#endif
