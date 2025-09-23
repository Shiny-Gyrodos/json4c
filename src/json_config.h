#ifndef JSON4C_CONFIG
#define JSON4C_CONFIG

#undef DEBUG
#ifdef JSON_DEBUG
#define DEBUG(msg, ...) printf("[%s:%d] " msg "\n", __FILE__, __LINE__,##__VA_ARGS__)
#else
#define DEBUG(msg) // Compile to nothing
#endif

#ifndef JSON_BUFFER_CAPACITY
#define JSON_BUFFER_CAPACITY 256
#endif
#ifndef JSON_DYNAMIC_ARRAY_CAPACITY
#define JSON_DYNAMIC_ARRAY_CAPACITY 16
#endif
#ifndef JSON_DYNAMIC_ARRAY_GROW_BY
#define JSON_DYNAMIC_ARRAY_GROW_BY 2
#endif
#ifndef JSON_MAX_ERRORS_RECORDED
#define JSON_MAX_ERRORS_RECORDED 64
#endif

#endif // JSON4C_CONFIG
