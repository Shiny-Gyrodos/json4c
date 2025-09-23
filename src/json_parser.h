#ifndef JSON4C_PARSER
#define JSON4C_PARSER

#include <stddef.h>
#include <stdarg.h>

#include "json_types.h"

JsonNode* json_parse(char* buffer, ptrdiff_t length);
JsonNode* json_parseFile(char* path);

JsonNode* json_property(JsonNode*, char*);
JsonNode* json_index(JsonNode*, ptrdiff_t);
#define json_get(node, ...) json_get_impl(node, __VA_ARGS__, -1)
JsonNode* json_get_impl(JsonNode*, ...); // NOTE: call the macro wrapper instead

#endif // JSON4C_PARSER
