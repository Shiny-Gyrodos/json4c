#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <math.h>
#include <inttypes.h>

#include "json_config.h"
#include "json_serializer.h"
#include "json_allocator.h"
#include "json_utils.h"

static void _serializeCondensed(JsonNode*, char**, ptrdiff_t*, ptrdiff_t*);
static void _serializePretty(JsonNode*, char**, ptrdiff_t*, ptrdiff_t*, char*, char*);


JsonNode* json_object_impl(void** ptrs) {
	JsonNode* jobject = json_node_create(NULL, (JsonValue){JSON_OBJECT, {0}});
	int i;
	char* identifier = NULL;
	for (i = 0; ptrs[i]; i++) {
		if (i % 2 == 0) { // starting with the first, every other value is an identifier (char*)
			identifier = (char*)ptrs[i];
		} else {
			JsonNode* jnode = (JsonNode*)ptrs[i];
			int length = strlen(identifier);
			void* temp = json_allocator.alloc(length + 1, json_allocator.context);
			if (!temp) {
				json_node_free(jobject);
				return NULL;
			}
			jnode->identifier = temp;
			strcpy(jnode->identifier, identifier);
			identifier = NULL;
			json_node_append(jobject, jnode);
		}
	}
	return jobject;
}

JsonNode* json_array_impl(JsonNode** jnodes) {
	JsonNode* jarray = json_node_create(NULL, (JsonValue){JSON_ARRAY, {0}});
	int i;
	for (i = 0; jnodes[i]; i++) {
		json_node_append(jarray, jnodes[i]);
	}
	return jarray;
}

inline JsonNode* json_bool(bool boolean) {
	return json_node_create(NULL, (JsonValue){JSON_BOOL, .boolean = boolean});
}

inline JsonNode* json_int(int integer) {
	return json_node_create(NULL, (JsonValue){JSON_INT, .integer = integer});
}

inline JsonNode* json_real(double real) {
	return json_node_create(NULL, (JsonValue){JSON_REAL, .real = real});
}

inline JsonNode* json_null(void) {
	return json_node_create(NULL, (JsonValue){JSON_NULL, {0}});
}

inline JsonNode* json_string(char* string) {
	return json_node_create(NULL, (JsonValue){JSON_STRING, .string = string});
}


void json_write(JsonNode* node, enum JsonWriteOption option, char* buffer, ptrdiff_t length) {
	ptrdiff_t jsonLength = length;
	ptrdiff_t jsonOffset = 0;
	char* jsonBuffer = json_toBuffer(node, option, &jsonLength, &jsonOffset);
	if (jsonLength > length) {
		// TODO: fix silent error
		json_allocator.free(jsonBuffer, jsonLength, json_allocator.free);
		return;
	}
	memcpy(buffer, jsonBuffer, jsonLength);
}

void json_writeFile(JsonNode* node, enum JsonWriteOption option, char* path, char* mode) {
	FILE* stream = fopen(path, mode);
	char* jsonText = json_toString(node, option);
	fputs(jsonText, stream);
	fclose(stream);
}


char* json_toBuffer(JsonNode* node, enum JsonWriteOption option, ptrdiff_t* length, ptrdiff_t* offset) {
	char* buffer = json_allocator.alloc(*length, json_allocator.context);
	if (option == JSON_WRITE_PRETTY) {
		_serializePretty(node, &buffer, length, offset, "", "");
	} else if (option == JSON_WRITE_CONDENSED) {
		_serializeCondensed(node, &buffer, length, offset);
	} else {
		return NULL; // TODO: fix silent fail
	}
	return buffer;
}

char* json_toString(JsonNode* node, enum JsonWriteOption option) {
	ptrdiff_t length = JSON_BUFFER_CAPACITY;
	ptrdiff_t offset = 0;
	char* buffer = json_toBuffer(node, option, &length, &offset);
	if (offset >= length) {
		void* temp = json_allocator.realloc(buffer, length + 1, length, json_allocator.context);
		if (!temp) {
			json_allocator.free(buffer, length, json_allocator.context);
			return NULL;
		}
		buffer = temp;
	}
	buffer[offset++] = '\0';
	return buffer;
}


// TODO: functions needs some spring cleaning, and thourough testing.
// TODO: add support for pretty printing ( ' ', '\t', and '\n')
#define appendStr(buffer, length, offset, ...) json_utils_dynAppendStr(buffer, length, offset, __VA_ARGS__)
static void _serializeCondensed(JsonNode* node, char** buffer, ptrdiff_t* length, ptrdiff_t* offset) {
	switch (node->value.type) {
		case JSON_OBJECT:
			appendStr(buffer, length, offset, "{");
			for (size_t i = 0; i < node->value.jcomplex.count; i++) {
				appendStr(
					buffer, 
					length, 
					offset,
					"\"",
					node->value.jcomplex.nodes[i]->identifier,
					"\":"
				);
				_serializeCondensed(node->value.jcomplex.nodes[i], buffer, length, offset);
				if (i + 1 < node->value.jcomplex.count) {
					appendStr(buffer, length, offset, ",");
				}
			}
			appendStr(buffer, length, offset, "}");
			break;
		case JSON_ARRAY:
			appendStr(buffer, length, offset, "[");
			for (size_t i = 0; i < node->value.jcomplex.count; i++) {
				_serializeCondensed(node->value.jcomplex.nodes[i], buffer, length, offset);
				if (i + 1 < node->value.jcomplex.count) {
					appendStr(buffer, length, offset, ",");
				}
			}
			appendStr(buffer, length, offset, "]");
			break;
		case JSON_INT: {
			char tempBuffer[21]; // 20 characters is exactly enough to hold int64_t min-value
			sprintf(tempBuffer, "%" PRId64, node->value.integer);
			appendStr(buffer, length, offset, tempBuffer);
			break;
		}
		case JSON_REAL: {
			char tempBuffer[25]; // 24 characters is enough to hold a %g formatted double
			sprintf(tempBuffer, "%g", node->value.real);
			appendStr(buffer, length, offset, tempBuffer);
			break;
		}
		case JSON_STRING:
			appendStr(buffer, length, offset, "\"", node->value.string, "\"");
			break;
		case JSON_BOOL:
			appendStr(buffer, length, offset, node->value.boolean ? "true" : "false");
			break;
		case JSON_NULL:
			appendStr(buffer, length, offset, "null");
			break;
		case JSON_ERROR: 
			// NOTE: a JSON_ERROR typed node will always have an error message in the identifier
			// but will only sometimes have extra data in node->value.string
			appendStr(buffer, length, offset, node->identifier, node->value.string);
			break;
		default: // for numbers casted to JsonType
			break;
	}
}
	
static void _serializePretty
	(JsonNode* node, char** buffer, ptrdiff_t* length, ptrdiff_t* offset, char* indent, char* extra) {
	switch (node->value.type) {
		case JSON_OBJECT: {
			appendStr(buffer, length, offset, extra, "{\n");
			// TODO: replace slow solution
			char* newIndent = json_allocator.alloc(strlen(indent) + 2, json_allocator.context);
			sprintf(newIndent, "\t%s", indent);
			for (size_t i = 0; i < node->value.jcomplex.count; i++) {
				appendStr(
					buffer, 
					length, 
					offset,
					newIndent,
					"\"",
					node->value.jcomplex.nodes[i]->identifier,
					"\": "
				);
				_serializePretty(
					node->value.jcomplex.nodes[i], 
					buffer, 
					length, 
					offset, 
					newIndent,
					""
				);
				if (i + 1 < node->value.jcomplex.count) {
					appendStr(buffer, length, offset, ",");
				}
				appendStr(buffer, length, offset, "\n");
			}
			json_allocator.free(newIndent, strlen(newIndent) + 1, json_allocator.context);
			appendStr(buffer, length, offset, indent, "}");
			break;
		}
		case JSON_ARRAY: {
			appendStr(buffer, length, offset, extra, "[\n");
			// TODO: replace slow solution
			char* newIndent = json_allocator.alloc(strlen(indent) + 2, json_allocator.context);
			sprintf(newIndent, "\t%s", indent);
			for (size_t i = 0; i < node->value.jcomplex.count; i++) {
				appendStr(buffer, length, offset, newIndent);
				_serializePretty(node->value.jcomplex.nodes[i], buffer, length, offset, newIndent, newIndent);
				if (i + 1 < node->value.jcomplex.count) {
					appendStr(buffer, length, offset, ",");
				}
				appendStr(buffer, length, offset, "\n");
			}
			json_allocator.free(newIndent, strlen(newIndent) + 1, json_allocator.context);
			appendStr(buffer, length, offset, indent, "]");
			break;
		}
		case JSON_INT: {
			char tempBuffer[21];
			sprintf(tempBuffer, "%" PRId64, node->value.integer);
			appendStr(buffer, length, offset, tempBuffer);
			break;
		}
		case JSON_REAL: {
			char tempBuffer[25];
			sprintf(tempBuffer, "%g", node->value.real);
			appendStr(buffer, length, offset, tempBuffer);
			break;
		}
		case JSON_STRING:
			appendStr(buffer, length, offset, "\"", node->value.string, "\"");
			break;
		case JSON_BOOL:
			appendStr(buffer, length, offset, node->value.boolean ? "true" : "false");
			break;
		case JSON_NULL:
			appendStr(buffer, length, offset, "null");
			break;
		case JSON_ERROR: 
			appendStr(buffer, length, offset, node->identifier, node->value.string);
			break;
		default:
			break;
	}
}
#undef appendStr
