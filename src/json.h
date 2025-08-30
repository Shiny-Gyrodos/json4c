#ifndef JSON4C_GUARD
#define JSON4C_GUARD

#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>

// NOTE: Only JSON_OBJECT and JSON_ARRAY are ever combined.
typedef enum {
	JSON_OBJECT,
	JSON_ARRAY,
	JSON_INT,
	JSON_REAL,
	JSON_STRING,
	JSON_BOOL,
	JSON_NULL,
	JSON_INVALID
} JsonType;

typedef struct JsonValue {
	JsonType type;
	union {
		int integer;
		double real;
		bool boolean;
		char* string;
		struct {
			struct JsonNode** nodes;
			size_t max;
			size_t count;
		} jcomplex;
	};
} JsonValue;

// Casts a JsonNode*
#define AS_INT(jnode)		((jnode)->value.integer)
#define AS_REAL(jnode)		((jnode)->value.real)
#define AS_BOOL(jnode)		((jnode)->value.boolean)
#define AS_STRING(jnode)	((jnode)->value.string)
#define AS_ARRAY(jnode)		((jnode)->value.jcomplex)
#define AS_OBJECT(jnode)	((jnode)->value.jcomplex)
// Tests a JsonNode*
#define IS_INT(jnode)		((jnode)->value.type == JSON_INT)
#define IS_REAL(jnode)		((jnode)->value.type == JSON_REAL)
#define IS_BOOL(jnode)		((jnode)->value.type == JSON_BOOL)
#define IS_STRING(jnode)	((jnode)->value.type == JSON_STRING)
#define IS_ARRAY(jnode)		((jnode)->value.type == JSON_ARRAY)
#define IS_OBJECT(jnode)	((jnode)->value.type == JSON_OBJECT)

typedef struct JsonNode {
	char* identifier;
	struct JsonValue value;
} JsonNode;

// Low-level node editing.
JsonNode* jnode_create(char*, JsonValue);
void jnode_append(JsonNode*, JsonNode*);
void jnode_free(JsonNode*);

// Serialization
JsonNode* json_object_impl(JsonNode**); // shouldn't be called, use the macro wrapper instead
#define json_object(...) json_object_impl((JsonNode*[]){__VA_ARGS__, NULL})
#define json_emptyObject() jnode_create(NULL, (JsonValue){JSON_OBJECT, 0})
JsonNode* json_array_impl(JsonNode**); // shouldn't be called, use the macro wrapper instead
#define json_array(...) json_array_impl((JsonNode*[]){__VA_ARGS__, NULL})
#define json_emptyArray() jnode_create(NULL, (JsonValue){JSON_ARRAY, 0})
JsonNode* json_bool(bool);
JsonNode* json_int(int);
JsonNode* json_real(double);
JsonNode* json_null(void);
JsonNode* json_string(char*);
bool json_write(char* path, JsonNode* jnode, char* indent);

// Deserialization
JsonNode* json_parse(char* buffer, ptrdiff_t length);
JsonNode* json_parseFile(char* path);
JsonNode* json_property(JsonNode*, char*);
JsonNode* json_index(JsonNode*, int);
JsonNode* json_get(JsonNode*, size_t, ...);

// Customization
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
// WARNING: Allocator should only be set before json nodes are created or after they are freed.
void json_setAllocator(void* (*custom_alloc)(void*, ptrdiff_t), void (*custom_free)(void*, void*, ptrdiff_t));
void json_resetAllocator(void);

#endif // JSON4C_GUARD
