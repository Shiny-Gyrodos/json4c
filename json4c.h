#ifndef JSON4C_GUARD
#define JSON4C_GUARD

#include <stdlib.h>
#include <stdbool.h>

typedef enum {
	JSON_OBJECT = 1 << 0, // We really only ever want to combine JSON_OBJECT and JSON_ARRAY
	JSON_ARRAY = 1 << 1,
	JSON_INT = 1 << 2,
	JSON_REAL = 1 << 3,
	JSON_STRING = 1 << 4,
	JSON_BOOL = 1 << 5,
	JSON_NULL = 1 << 6,
	JSON_INVALID = 1 << 7,
} JsonType;

#define JSON_COMPLEX (JSON_OBJECT | JSON_ARRAY)
#define IS_COMPLEX(jsonType) (jsonType & JSON_COMPLEX)

#ifndef JSON_COMPLEX_DEFAULT_CAPACITY
#define JSON_COMPLEX_DEFAULT_CAPACITY 16
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

JsonNode* jnode_create(char*, JsonValue);
void jnode_append(JsonNode*, JsonNode*);
void jnode_free(JsonNode*);

JsonNode* json_parseFile(char*);
JsonNode* json_get(JsonNode*, size_t, ...);

#endif // JSON4C_GUARD
