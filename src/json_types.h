/*
	This file doesn't represent ALL the types in this library,
	rather the common types and a few functions to operate on them.
*/

#ifndef JSON4C_TYPES
#define JSON4C_TYPES

#include "json_allocator.h"

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
	enum JsonType type;
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

bool json_type_isComplex(JsonType);

JsonNode* json_node_create(char*, JsonValue, Allocator);
JsonNode* json_node_append(JsonNode*, JsonNode*, Allocator);
JsonNode* json_node_free(JsonNode*, Allocator);

#endif // JSON4C_TYPES
