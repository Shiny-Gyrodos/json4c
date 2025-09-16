#ifndef JSON4C_SERIALIZER
#define JSON4C_SERIALIZER

#include <stdbool.h>
#include <stddef.h>

#include "json_types.h"

typedef enum {
	JSON_WRITE_PRETTY,
	JSON_WRITE_COMPRESSED
} JsonWriteOption;

JsonNode* json_object_impl(void**); // shouldn't be called, use the macro wrapper instead
#define json_object(...) json_object_impl((void*[]){__VA_ARGS__, NULL})
#define json_emptyObject() jnode_create(NULL, (JsonValue){JSON_OBJECT, 0})
JsonNode* json_array_impl(JsonNode**); // shouldn't be called, use the macro wrapper instead
#define json_array(...) json_array_impl((JsonNode*[]){__VA_ARGS__, NULL})
#define json_emptyArray() jnode_create(NULL, (JsonValue){JSON_ARRAY, 0})
JsonNode* json_bool(bool);
JsonNode* json_int(int);
JsonNode* json_real(double);
JsonNode* json_null(void);
JsonNode* json_string(char*);

void json_write(JsonNode* jnode, JsonWriteOption option, char* buffer, ptrdiff_t length);
void json_writeFile(JsonNode* jnode, JsonWriteOption option, char* path, char* mode);

int json_charLength(JsonNode*, JsonWriteOption);
char* json_toString(JsonNode*, JsonWriteOption);

#endif // JSON4C_SERIALIZER
