#ifndef JSON4C_SERIALIZER
#define JSON4C_SERIALIZER

#include <stdbool.h>
#include <stddef.h>

#include "json_types.h"

enum JsonWriteOption {
	JSON_WRITE_PRETTY,
	JSON_WRITE_CONDENSED
};

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

bool json_write(JsonNode* jnode, enum JsonWriteOption option, char* buffer, ptrdiff_t length);
void json_writeFile(JsonNode* jnode, enum JsonWriteOption, char* path, char* mode);

/* 
	NOTE:
	These two function behave similiarly. The difference being 'json_toString'
	returns a valid C string, while 'json_toBuffer' produces a buffer of bytes
	and assigns *length to the buffer length and *offset to n + 1 where n is 
	the amount of bytes in the buffer.
*/
char* json_toBuffer(JsonNode* node, enum JsonWriteOption, ptrdiff_t* length, ptrdiff_t* offset);
char* json_toString(JsonNode* node, enum JsonWriteOption);

#endif // JSON4C_SERIALIZER
