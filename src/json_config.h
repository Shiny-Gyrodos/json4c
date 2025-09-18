#ifndef JSON4C_CONFIG
#define JSON4C_CONFIG

#undef DEBUG
#ifdef JSON_DEBUG
#define DEBUG(msg, ...) printf("[%s:%d] " msg "\n", __FILE__, __LINE__,##__VA_ARGS__)
#else
#define DEBUG(msg) // Compile to nothing
#endif

#ifndef JSON_BUFFER_DEFAULT
#define JSON_BUFFER_DEFAULT 256
#endif
#ifndef JSON_COMPLEX_CAPACITY
#define JSON_COMPLEX_CAPACITY 16
#endif
#ifndef JSON_COMPLEX_GROW_MULTIPLIER
#define JSON_COMPLEX_GROW_MULTIPLIER 2
#endif

#endif // JSON4C_CONFIG
