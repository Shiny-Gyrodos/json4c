#ifndef JSON4C_CONFIG
#define JSON4C_CONFIG

#undef DEBUG
#ifdef JSON4C_DEBUG
#define DEBUG(msg) printf("[%s:%d] %s\n", __FILE__, __LINE__, msg)
#else
#define DEBUG(msg) // Compile to nothing
#endif

#ifndef JSON_COMPLEX_DEFAULT_CAPACITY
#define JSON_COMPLEX_DEFAULT_CAPACITY 16
#endif
#ifndef JSON_COMPLEX_GROW_MULTIPLIER
#define JSON_COMPLEX_GROW_MULTIPLIER 2
#endif
#ifndef JSON_DEFAULT_ALLOC
#define JSON_DEFAULT_ALLOC std_malloc // malloc wrapper
#endif
#ifndef JSON_DEFAULT_FREE
#define JSON_DEFAULT_FREE std_free // free wrapper
#endif

#endif // JSON4C_CONFIG
