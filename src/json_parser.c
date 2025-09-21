#include <stdbool.h>
#include <stdint.h>
#include <math.h.>
#include <ctype.h>
#include <stdio.h>

#include "json_parser.h"
#include "json_types.h"
#include "json_config.h"
#include "json_utils.h"

// TODO: String parser needs extra work to support escaped characters.
// TODO: Remove _scanUntil and replace it with a function tailored to string parsing
// TODO: _error should return something some JsonNode* with type of JSON_ERROR
// and _skip should return NULL instead as to not waste resources.
// the library should provide a function for seeing if a parse was successful,
// and if not, extracting the error.


// Parsers
typedef JsonNode* (*parserFunc)(char*, ptrdiff_t, ptrdiff_t*);
typedef JsonNode* parser(char*, ptrdiff_t, ptrdiff_t*);
static parser _error;
static parser _skip;
static parser _object;
static parser _array;
static parser _boolean;
static parser _string;
static parser _number;
static parser _null;

// Helpers
static parserFunc _getParser(char character);
static char* _scanUntil(char*, char*, ptrdiff_t, ptrdiff_t*);
static char* _scanWhile(bool (*predicate)(char), char*, ptrdiff_t, ptrdiff_t*);


JsonNode* json_parse(char* buffer, ptrdiff_t length) {
	if (length <= 0) return NULL;
	ptrdiff_t offset = 0;
	parserFunc firstParser = _getParser(json_buf_peek(buffer, length, offset));
	JsonNode* root = firstParser(buffer, length, &offset);
	return root;
}

JsonNode* json_parseFile(char* path) {
	FILE* jsonStream = fopen(path, "rb");
	if (!jsonStream) return NULL;
	fseek(jsonStream, 0, SEEK_END);
	long length = ftell(jsonStream);
	rewind(jsonStream);
	
	char* buffer = json_allocator.alloc(length + 1, json_allocator.context);
	if (!buffer) {
		fclose(jsonStream);
		return NULL;
	}
	size_t bytesRead = fread(buffer, 1, length, jsonStream);
	fclose(jsonStream);
	buffer[bytesRead] = '\0';
	DEBUG("file contents:\n%s", buffer);
	
	return json_parse(buffer, length);
}

JsonNode* json_property(JsonNode* jnode, char* identifier) {
	if (!jnode || !identifier || jnode->value.type != JSON_OBJECT) return NULL;
	size_t i;
	for (i = 0; i < jnode->value.jcomplex.count; i++) {
		if (strcmp(jnode->value.jcomplex.nodes[i]->identifier, identifier) == 0) {
			return jnode->value.jcomplex.nodes[i];
		}
	}
	return NULL;
}

JsonNode* json_index(JsonNode* jnode, size_t index) {
	if (!jnode || jnode->value.type != JSON_ARRAY || index >= jnode->value.jcomplex.count) 
		return NULL;
	return jnode->value.jcomplex.nodes[index];
}

#define TERMINATOR -1
JsonNode* json_get_impl(JsonNode* root, ...) {
	va_list args;
	va_start(args, root);
	while (true) {
		if (!root) {
			va_end(args);
			return NULL;
		}
		if (root->value.type == JSON_OBJECT) {
			char* identifier = va_arg(args, char*);
			if (identifier == (char*)TERMINATOR) 
				break;
			root = json_property(root, identifier);
		} else if (root->value.type == JSON_ARRAY) {
			intptr_t index = va_arg(args, intptr_t);
			if (index == TERMINATOR) 
				break;
			root = json_index(root, index);
		} else {
			if (va_arg(args, intptr_t) == TERMINATOR) 
				break;
			va_end(args);
			return NULL;
		}
	}
	va_end(args);
	return root;
}
#undef TERMINATOR

// Predicates
static bool _numberPredicate(char c) { // TODO: fix bandaid fix
	return c == '.' || c == 'e' || c == 'E' || c == '+' || c == '-' || isdigit(c); 
}
static bool _letterPredicate(char c) { return isalpha(c); }
static bool _errorPredicate(char c) { return _getParser(c) == _error; }

static JsonNode* _error(char* buffer, ptrdiff_t length, ptrdiff_t* offset) {
	char* string = _scanWhile(_errorPredicate, buffer, length, offset);
	return json_node_create("JSON_ERROR: unexpected character(s) ", (JsonValue){JSON_ERROR, .string = string});
}

// NOTE: a parser returning NULL means an unimportant character was parsed, a parser that fails returns JSON_ERROR
static JsonNode* _skip(char* buffer, ptrdiff_t length, ptrdiff_t* offset) {
	DEBUG("( %c ) skipped", json_buf_get(buffer, length, offset));
	return NULL;
}

static JsonNode* _object(char* buffer, ptrdiff_t length, ptrdiff_t* offset) {
	if (!json_buf_expect('{', buffer, length, offset)) 
		return json_node_create("JSON_ERROR: ( { ) missing ", (JsonValue){JSON_ERROR, .string = NULL});
	DEBUG("( { ) parsed");
	JsonNode* jobject = json_node_create(NULL, (JsonValue){JSON_OBJECT, {0}});
	char* identifier = NULL;
	char nextChar;
	while ((nextChar = json_buf_peek(buffer, length, *offset)) != '}' && *offset < length) {
		parserFunc currentParser = _getParser(nextChar);
		JsonNode* appendee = currentParser(buffer, length, offset);
		if (!appendee) {
			continue;
		} else if (appendee->value.type == JSON_ERROR) { // TODO: this is wasteful, FIX
			json_node_free(jobject);
			DEBUG("parser returned JSON_ERROR, bailing out");
			return appendee;
		} else if (!identifier && appendee->value.type == JSON_STRING) {
			size_t stringLength = strlen(appendee->value.string);
			identifier = json_allocator.alloc(stringLength, json_allocator.context);
			memcpy(identifier, appendee->value.string, stringLength);
			json_node_free(appendee);
		} else {
			appendee->identifier = identifier;
			json_node_append(jobject, appendee);
			identifier = NULL;
		}
	}
	if (!json_buf_expect('}', buffer, length, offset)) {
		json_node_free(jobject);
		DEBUG("( } ) missing");
		return json_node_create("JSON_ERROR: ( } ) missing ", (JsonValue){JSON_ERROR, .string = NULL});
	}
	DEBUG("( } ) parsed");
	return jobject;
}

static JsonNode* _array(char* buffer, ptrdiff_t length, ptrdiff_t* offset) {
	if (!json_buf_expect('[', buffer, length, offset))
		json_node_create("JSON_ERROR: ( [ ) missing ", (JsonValue){JSON_ERROR, .string = NULL});
	DEBUG("( [ ) parsed");
	JsonNode* jarray = json_node_create(NULL, (JsonValue){JSON_ARRAY, {0}});
	char nextChar;
	while ((nextChar = json_buf_peek(buffer, length, *offset)) != ']' && *offset < length) {
		parserFunc currentParser = _getParser(nextChar);
		JsonNode* appendee = currentParser(buffer, length, offset);
		if (!appendee) {
			continue;
		} else if (appendee->value.type == JSON_ERROR) {
			json_node_free(jarray);
			DEBUG("parser returned JSON_ERROR, bailing out");
			return appendee; // TODO: better error reporting
		}
		json_node_append(jarray, appendee);
	}
	if (!json_buf_expect(']', buffer, length, offset)) {
		json_node_free(jarray);
		return json_node_create("JSON_ERROR: ( ] ) missing ", (JsonValue){JSON_ERROR, .string = NULL});
	}
	DEBUG("( ] ) parsed");
	return jarray;
}

static JsonNode* _boolean(char* buffer, ptrdiff_t length, ptrdiff_t* offset) {
	char* boolString = _scanWhile(_letterPredicate, buffer, length, offset);
	JsonNode* jnode;
	if (strcmp(boolString, "true") == 0) {
		DEBUG("( true ) parsed");
		jnode = json_node_create(NULL, (JsonValue){JSON_BOOL, .boolean = true});
		json_allocator.free(boolString, strlen(boolString), json_allocator.context);
	} else if (strcmp(boolString, "false") == 0) {
		DEBUG("( false ) parsed");
		jnode = json_node_create(NULL, (JsonValue){JSON_BOOL, .boolean = false});
		json_allocator.free(boolString, strlen(boolString), json_allocator.context);
	} else {
		jnode = json_node_create("JSON_ERROR: unexpected character(s) ", (JsonValue){JSON_ERROR, .string = boolString});
	}
	return jnode;
}

static JsonNode* _string(char* buffer, ptrdiff_t length, ptrdiff_t* offset) {
	json_buf_get(buffer, length, offset); // Not json_bufexpect because at this point we know it's '"'
	char* string = _scanUntil("\"", buffer, length, offset);
	DEBUG("( \"%s\" ) parsed", string);
	if (!string)
		return json_node_create("JSON_ERROR: out of memory ", (JsonValue){JSON_ERROR, .string = NULL});
	json_buf_get(buffer, length, offset);
	return json_node_create(NULL, (JsonValue){JSON_STRING, .string = string});
}

// TODO: _number isn't compliant with the JSON standard
static JsonNode* _number(char* buffer, ptrdiff_t length, ptrdiff_t* offset) {
	char* numString = _scanWhile(_numberPredicate, buffer, length, offset);
	if (!numString)
		return json_node_create("JSON_ERROR: out of memory ", (JsonValue){JSON_ERROR, .string = NULL});
	char* end;
	double real = strtod(numString, &end);
	*offset -= &numString[strlen(numString) - 1] - end;
	double integer;
	json_allocator.free(numString, strlen(numString), json_allocator.context);
	if (modf(real, &integer) == 0.0) {
		DEBUG("( %d ) parsed", (int)integer);
		return json_node_create(NULL, (JsonValue){JSON_INT, .integer = (int64_t)integer});
	}
	DEBUG("( %lf ) parsed", real);
	return json_node_create(NULL, (JsonValue){JSON_REAL, .real = real});
}

static JsonNode* _null(char* buffer, ptrdiff_t length, ptrdiff_t* offset) {
	char* nullString = _scanWhile(_letterPredicate, buffer, length, offset);
	if (!nullString)
		return json_node_create("JSON_ERROR: out of memory ", (JsonValue){JSON_ERROR, .string = NULL});
	JsonNode* jnode = NULL;
	if (strcmp(nullString, "null") == 0) {
		DEBUG("( null ) parsed");
		jnode = json_node_create(NULL, (JsonValue){JSON_NULL, {0}});
	}
	json_allocator.free(nullString, strlen(nullString), json_allocator.context);
	return jnode;
}

static parserFunc _getParser(char character) {
	switch (character) {
		case '{':
		case '}':
			return _object;
		case '[':
		case ']':
			return _array;
		case '"':
			return _string;
		case ',':
		case ':':
			return _skip;
		case 't': // true
		case 'f': // false
			return _boolean;
		case 'n': // null
			return _null;
		default: // number or whitespace
			if (isdigit(character)) {
				return _number;
			} else if (isspace(character)) {
				return _skip;
			}
			return _error;
	}
}

char* _scanUntil(char* delimiters, char* buffer, ptrdiff_t length, ptrdiff_t* offset) {
	if (!delimiters || !buffer || !offset) return NULL;
	ptrdiff_t max = JSON_DYNAMIC_ARRAY_CAPACITY;
	ptrdiff_t current = 0;
	char* string = json_allocator.alloc(max, json_allocator.context);
	if (!string) return NULL;
	char currentChar;
	while (*offset < length && !strchr(delimiters, currentChar = json_buf_get(buffer, length, offset))) {
		if (current >= max - 2) { // minus 2 to leave room for null terminating character
			char* temp = json_allocator.realloc(string, max * 2, max, json_allocator.context);
			if (!temp) return NULL;
			string = temp;
			max *= 2;
		}
		string[current++] = currentChar;
	}
	json_buf_put(currentChar, buffer, length, offset);
	string[current] = '\0';
	return string; 
}

char* _scanWhile(bool (*predicate)(char), char* buffer, ptrdiff_t length, ptrdiff_t* offset) {
	if (!predicate || !buffer || !offset) return NULL;
	ptrdiff_t max = JSON_DYNAMIC_ARRAY_CAPACITY;
	ptrdiff_t current = 0;
	char* string = json_allocator.alloc(max, json_allocator.context);
	if (!string) return NULL;
	char currentChar;
	while (*offset < length && predicate(currentChar = json_buf_get(buffer, length, offset))) {
		if (current >= max - 2) { // minus 2 to leave room for null terminating character
			char* temp = json_allocator.realloc(string, max * 2, max, json_allocator.context);
			if (!temp) return NULL;
			string = temp;
			max *= 2;
		}
		string[current++] = currentChar;
	}
	json_buf_put(currentChar, buffer, length, offset);
	string[current] = '\0';
	return string;
}
