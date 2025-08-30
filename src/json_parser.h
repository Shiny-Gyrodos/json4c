#ifndef JSON4C_PARSER
#define JSON4C_PARSER

#include <stddef.h>

JsonNode* json_parse(char* buffer, ptrdiff_t length, ptrdiff_t offset);
JsonNode* json_parseFile(char* path);
JsonNode* json_property(JsonNode*, char*);
JsonNode* json_index(JsonNode*, int);
JsonNode* json_get(JsonNode*, ptrdiff_t, ...);

#endif // JSON4C_PARSER
