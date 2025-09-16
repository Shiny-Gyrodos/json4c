/*
	This file doesn't represent ALL the types in this library,
	rather the common types and a few functions to operate on them.
*/

#ifndef JSON4C_TYPES
#define JSON4C_TYPES

#include <stdbool.h>

#include "json_allocator.h"

typedef enum {
	JSON_INVALID,
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
// Tests a JsonNode*
#define IS_INT(jnode)		(!(jnode) && (jnode)->value.type == JSON_INT)
#define IS_REAL(jnode)		(!(jnode) && (jnode)->value.type == JSON_REAL)
#define IS_BOOL(jnode)		(!(jnode) && (jnode)->value.type == JSON_BOOL)
#define IS_STRING(jnode)	(!(jnode) && (jnode)->value.type == JSON_STRING)
#define IS_ARRAY(jnode)		(!(jnode) && (jnode)->value.type == JSON_ARRAY)
#define IS_OBJECT(jnode)	(!(jnode) && (jnode)->value.type == JSON_OBJECT)
#define IS_ERROR(jnode)		(!(jnode) && (jnode)->value.type == JSON_ERROR)
#define IS_INVALID(jnode)	(!(jnode) && (jnode)->value.type == JSON_INVALID)

bool json_type_isComplex(JsonType);

JsonNode* json_node_create(char*, JsonValue);
void json_node_append(JsonNode*, JsonNode*);
void json_node_free(JsonNode*);

#endif // JSON4C_TYPES
