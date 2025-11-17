/*
	This file doesn't represent ALL the types in this library,
	rather the common types and a few functions to operate on them.
*/

#ifndef JSON4C_TYPES
#define JSON4C_TYPES

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "json_allocator.h"

typedef enum {
	JSON_ERROR,
	JSON_OBJECT,
	JSON_ARRAY,
	JSON_INT,
	JSON_REAL,
	JSON_STRING,
	JSON_BOOL,
	JSON_NULL
} JsonType;

typedef struct JsonValue {
	JsonType type;
	union {
		int64_t integer;
		double real;
		bool boolean;
		char* string;
		struct {
			struct JsonNode** nodes;
			ptrdiff_t max;
			ptrdiff_t count;
		} jcomplex;
	};
} JsonValue;

typedef struct JsonNode {
	char* identifier;
	struct JsonValue value;
} JsonNode;

// Casts a JsonNode*
#define AS_INT(jnode)		((jnode)->value.integer)
#define AS_REAL(jnode)		((jnode)->value.real)
#define AS_BOOL(jnode)		((jnode)->value.boolean)
#define AS_STRING(jnode)	((jnode)->value.string)
#define AS_ARRAY(jnode)		((jnode)->value.jcomplex)
#define AS_OBJECT(jnode)	((jnode)->value.jcomplex)
// This one exists mainly for implementation clarity
#define AS_COMPLEX(jnode)	((jnode)->value.jcomplex)

// Tests a JsonNode*
#define IS_INT(jnode)		((jnode) && (jnode)->value.type == JSON_INT)
#define IS_REAL(jnode)		((jnode) && (jnode)->value.type == JSON_REAL)
#define IS_BOOL(jnode)		((jnode) && (jnode)->value.type == JSON_BOOL)
#define IS_STRING(jnode)	((jnode) && (jnode)->value.type == JSON_STRING)
#define IS_NULL(jnode)		((jnode) && (jnode)->value.type == JSON_NULL)
#define IS_ARRAY(jnode)		((jnode) && (jnode)->value.type == JSON_ARRAY)
#define IS_OBJECT(jnode)	((jnode) && (jnode)->value.type == JSON_OBJECT)
#define IS_ERROR(jnode)		((jnode) && (jnode)->value.type == JSON_ERROR)

bool json_type_isComplex(JsonType);

JsonNode* json_node_create(char*, JsonValue);
void json_node_append(JsonNode*, JsonNode*);
ptrdiff_t json_node_childrenCount(const JsonNode*);
bool json_node_equals(const JsonNode*, const JsonNode*);
void json_node_free(JsonNode*);

JsonNode* json_object_impl(void**); // shouldn't be called, use the macro wrapper instead
#define json_object(...) json_object_impl((void*[]){__VA_ARGS__, NULL})
#define json_emptyObject() jnode_create(NULL, (JsonValue){JSON_OBJECT, 0})
JsonNode* json_array_impl(JsonNode**); // shouldn't be called, use the macro wrapper instead
#define json_array(...) json_array_impl((JsonNode*[]){__VA_ARGS__, NULL})
#define json_emptyArray() jnode_create(NULL, (JsonValue){JSON_ARRAY, 0})
JsonNode* json_bool(bool);
JsonNode* json_int(int64_t);
JsonNode* json_real(double);
JsonNode* json_null(void);
JsonNode* json_string(char*);

JsonNode* json_property(JsonNode*, char*);
JsonNode* json_index(JsonNode*, ptrdiff_t);
#define json_get(node, ...) json_get_impl(node, __VA_ARGS__, -1)
JsonNode* json_get_impl(JsonNode*, ...); // NOTE: call the macro wrapper instead

#endif // JSON4C_TYPES
