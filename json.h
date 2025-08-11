#ifndef C_JSON
#define C_JSON

#include <stdlib.h>
#include <stdbool.h>

typedef enum {
	JSON_INVALID = -1,
	JSON_OBJECT = 1 << 0,
	JSON_ARRAY = 1 << 1,
	JSON_INT,
	JSON_REAL,
	JSON_STRING,
	JSON_BOOL,
	JSON_NULL
} JsonType;

#define JSON_COMPLEX (JSON_OBJECT | JSON_ARRAY)

union JsonValue {
	int integer;
	float real;
	bool boolean;
	char* string;
	struct {
		struct JsonNode** nodes;
		size_t max;
		size_t count;
	} jsonComplex;
};

typedef struct JsonNode {
	struct JsonNode* parent;
	char* identifier;
	JsonType type;
	union JsonValue value;
} JsonNode;

JsonNode* jnode_create(JsonNode*, char*, JsonType);
void jnode_append(JsonNode*, JsonNode*);
void jnode_free(JsonNode*);

JsonNode* json_parseFile(char*);
bool json_isValid(char*);
JsonNode* json_get(JsonNode*, size_t, ...);

#endif // C_JSON
