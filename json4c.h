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
#define IS_COMPLEX(jsonType) (jsonType & JSON_COMPLEX)

// Users should define these as desired.
#ifndef JSON_COMPLEX_DEFAULT_CAPACITY
#define JSON_COMPLEX_DEFAULT_CAPACITY 16
#endif
#ifndef JSON_COMPLEX_GROW_MULTIPLIER
#define JSON_COMPLEX_GROW_MULTIPLIER 2
#endif

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
JsonNode* _json_array(JsonNode**);
#define json_array(...) _json_array((JsonNode*[]){__VA_ARGS__, NULL})
JsonNode* json_bool(bool);
JsonNode* json_int(int);
JsonNode* json_real(float);
JsonNode* json_null(void);
JsonNode* json_string(char*);
bool json_write(char* path, JsonNode* jnode, char* indent);

// Deserialization
JsonNode* json_parseFile(char* path);
JsonNode* json_get(JsonNode*, size_t, ...);

#endif // JSON4C_GUARD
