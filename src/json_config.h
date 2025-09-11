#ifndef JSON4C_CONFIG
#define JSON4C_CONFIG

#undef DEBUG
#ifdef JSON4C_DEBUG
#define DEBUG(msg, ...) printf("[%s:%d] " msg "\n", __FILE__, __LINE__,##__VA_ARGS__)
#else
#define DEBUG(msg) // Compile to nothing
#endif

#ifndef JSON_COMPLEX_CAPACITY
#define JSON_COMPLEX_CAPACITY 16
#endif
#ifndef JSON_COMPLEX_GROW_MULTIPLIER
#define JSON_COMPLEX_GROW_MULTIPLIER 2
#endif
#ifndef JSON_ALLOCATOR_DEFAULT
#define JSON_ALLOCATOR_DEFAULT (struct Allocator){json_std_alloc, json_std_free, json_std_realloc, NULL}
#endif

#endif // JSON4C_CONFIG
