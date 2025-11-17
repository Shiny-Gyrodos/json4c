#ifndef JSON4C_PARSER
#define JSON4C_PARSER

#include <stddef.h>
#include <stdarg.h>

#include "json_types.h"

JsonNode* json_parse(char* buffer, ptrdiff_t length);
JsonNode* json_parseFile(char* path);

#endif // JSON4C_PARSER
