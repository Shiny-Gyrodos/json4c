#ifndef JSON4C_GUARD
#define JSON4C_GUARD

#include <stdlib.h>
#include <stdbool.h>

// NOTE: Only JSON_OBJECT and JSON_ARRAY are ever combined.
typedef enum {
	JSON_OBJECT 	=	1 << 0,
	JSON_ARRAY 		= 	1 << 1,
	JSON_INT 		= 	1 << 2,
	JSON_REAL 		= 	1 << 3,
	JSON_STRING 	= 	1 << 4,
	JSON_BOOL 		= 	1 << 5,
	JSON_NULL 		= 	1 << 6,
	JSON_INVALID 	= 	1 << 7
} JsonType;

#define JSON_COMPLEX (JSON_OBJECT | JSON_ARRAY)

typedef struct JsonValue {
	JsonType type;
	union {
		int integer;
		float real;
		bool boolean;
		char* string;
		struct {
			struct JsonNode** nodes;
			size_t max;
			size_t count;
		} jcomplex;
	};
} JsonValue;

#define AS_INT(jnode)		((jnode)->value.integer)
#define AS_REAL(jnode)		((jnode)->value.real)
#define AS_BOOL(jnode)		((jnode)->value.boolean)
#define AS_STRING(jnode)	((jnode)->value.string)
#define AS_ARRAY(jnode)		((jnode)->value.jcomplex)
#define AS_OBJECT(jnode)	((jnode)->value.jcomplex)
#define IS_INT(jnode)		((jnode)->value.type == JSON_INT)
#define IS_REAL(jnode)		((jnode)->value.type == JSON_REAL)
#define IS_BOOL(jnode)		((jnode)->value.type == JSON_BOOL)
#define IS_STRING(jnode)	((jnode)->value.type == JSON_STRING)
#define IS_ARRAY(jnode)		((jnode)->value.type == JSON_ARRAY)
#define IS_OBJECT(jnode)	((jnode)->value.type == JSON_OBJECT)
#define IS_COMPLEX(jsonType) ((jsonType) & JSON_COMPLEX)

typedef struct JsonNode {
	char* identifier;
	struct JsonValue value;
} JsonNode;

// Low-level node editing.
JsonNode* jnode_create(char*, JsonValue);
void jnode_append(JsonNode*, JsonNode*);
void jnode_free(JsonNode*);

// Serialization
JsonNode* _json_object(JsonNode**);
#define json_object(...) _json_object((JsonNode*[]){__VA_ARGS__, NULL})
#define json_emptyObject() jnode_create(NULL, (JsonValue){JSON_OBJECT, 0})
JsonNode* _json_array(JsonNode**);
#define json_array(...) _json_array((JsonNode*[]){__VA_ARGS__, NULL})
#define json_emptyArray() jnode_create(NULL, (JsonValue){JSON_ARRAY, 0})
JsonNode* json_bool(bool);
JsonNode* json_int(int);
JsonNode* json_real(float);
JsonNode* json_null(void);
JsonNode* json_string(char*);
bool json_write(char* path, JsonNode* jnode, char* indent);

// Deserialization
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
#define JSON_DEFAULT_ALLOC malloc
#endif
#ifndef JSON_DEFAULT_FREE
#define JSON_DEFAULT_FREE free
#endif
// WARNING: Allocator should only be set before json nodes are created or after they are freed.
void json_setAllocator(void* (*json_alloc)(size_t), void (*json_free)(void*));
void json_resetAllocator(void);

#endif // JSON4C_GUARD
