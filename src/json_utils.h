#ifndef JSON4C_UTILS
#define JSON4C_UTILS

#include "json_allocator.h"

char* json_utils_scanUntil(char*, char*, Allocator);
char* json_utils_scanWhile(char*, bool (predicate)(char), Allocator);

#endif // JSON4C_UTILS
