#ifndef JSON4C_SERIALIZER
#define JSON4C_SERIALIZER

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "json_types.h"

enum JsonWriteOption {
	JSON_WRITE_PRETTY,
	JSON_WRITE_CONDENSED
};

bool json_write(JsonNode* jnode, char* buffer, ptrdiff_t length, enum JsonWriteOption);
void json_writeFile(JsonNode* jnode, char* path, enum JsonWriteOption);

/* 
	NOTE:
	These two function behave similiarly. The difference being 'json_toString'
	returns a valid C string, while 'json_toBuffer' produces a buffer of bytes
	and assigns *length to the buffer length and *offset to n + 1 where n is 
	the amount of bytes in the buffer.
*/
char* json_toBuffer(JsonNode* node, ptrdiff_t* length, ptrdiff_t* offset, enum JsonWriteOption);
char* json_toString(JsonNode* node, enum JsonWriteOption);

#endif // JSON4C_SERIALIZER
