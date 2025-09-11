#include <stdbool.h>
#include <math.h.>
#include <ctype.h>
#include <stdio.h>

#include "json_parser.h"
#include "json_types.h"

// TODO: Number parser needs extra work to support numbers like -11.86e2
// TODO: String parser needs extra work to support escaped characters.

// Parsers
typedef JsonNode* (*parserFunc)(char*, ptrdiff_t, ptrdiff_t*);
typedef JsonNode* parser(char*, ptrdiff_t, ptrdiff_t*);
static parser _skip;
static parser _object;
static parser _array;
static parser _boolean;
static parser _string;
static parser _number;
static parser _null;

// Helpers
static parserFunc _getParser(char character);
static bool _bufexpect(char, char*, ptrdiff_t, ptrdiff_t*);
static char _bufget(char*, ptrdiff_t, ptrdiff_t*);
static char _bufput(char, char*, ptrdiff_t, ptrdiff_t*);
static char _bufpeek(char*, ptrdiff_t, ptrdiff_t);
static char* _scanUntil(char*, char*, ptrdiff_t, ptrdiff_t*);
static char* _scanWhile(bool (*predicate)(char), char*, ptrdiff_t, ptrdiff_t*);


JsonNode* json_parse(char* buffer, ptrdiff_t length) {
	if (length <= 0) return NULL;
	ptrdiff_t offset = 0;
	parserFunc firstParser = _getParser(_bufpeek(buffer, length, offset));
	JsonNode* root = firstParser(buffer, length, &offset);
	return root;
}

JsonNode* json_parseFile(char* path) {
	FILE* jsonStream = fopen(path, "r");
	if (!jsonStream) return NULL;
	fseek(jsonStream, 0, SEEK_END);
	long length = ftell(jsonStream);
	rewind(jsonStream);
	
	char* buffer = json_allocator.alloc(length, json_allocator.context);
	if (!buffer) {
		fclose(jsonStream);
		return NULL;
	}
	fread(buffer, 1, length, jsonStream);
	fclose(jsonStream);
	
	return json_parse(buffer, length);
}

JsonNode* json_property(JsonNode* jnode, char* identifier) {
	if (!jnode || jnode->value.type != JSON_OBJECT) return NULL;
	int i;
	for (i = 0; i < jnode->value.jcomplex.count; i++) {
		if (strcmp(jnode->value.jcomplex.nodes[i]->identifier, identifier) == 0) {
			return jnode->value.jcomplex.nodes[i];
		}
	}
	return NULL;
}

JsonNode* json_index(JsonNode* jnode, int index) {
	if (!jnode || jnode->value.type != JSON_ARRAY || index >= jnode->value.jcomplex.count) return NULL;
	return jnode->value.jcomplex.nodes[index];
}

JsonNode* json_get(JsonNode* root, ptrdiff_t count, ...) {
	va_list args;
	va_start(args, count);
	while (count --> 0) {
		if (!root) return NULL;
		if (root->value.type == JSON_OBJECT) {
			root = json_property(root, va_arg(args, char*));
		} else if (root->value.type == JSON_ARRAY) {
			root = json_index(root, va_arg(args, int));
		} else {
			return NULL;
		}
	}
	va_end(args);
	return root;
}


// Predicates
static bool _numberPredicate(char c) { // TODO: fix bandaid fix
	return c == '.' || c == 'e' || c == 'E' || c == '+' || c == '-' || isdigit(c); 
}
static bool _letterPredicate(char character) { return isalpha(character); }

static JsonNode* _skip(char* buffer, ptrdiff_t length, ptrdiff_t* offset) {
	_bufget(buffer, length, offset);
	return json_node_create(NULL, (JsonValue){JSON_INVALID, 0});
}

static JsonNode* _object(char* buffer, ptrdiff_t length, ptrdiff_t* offset) {
	if (!_bufexpect('{', buffer, length, offset)) return NULL;
	JsonNode* jobject = json_node_create(NULL, (JsonValue){JSON_OBJECT, 0});
	char* identifier = NULL;
	char nextChar;
	while ((nextChar = _bufpeek(buffer, length, *offset)) != '}' && *offset < length) {
		parserFunc currentParser = _getParser(nextChar);
		JsonNode* appendee = currentParser(buffer, length, offset);
		if (!appendee) {
			json_node_free(jobject);
			json_node_free(appendee);
			return NULL;
		} else if (appendee->value.type == JSON_INVALID) { // TODO: this is wasteful, FIX
			json_node_free(appendee);
			continue;
		} else if (!identifier && appendee->value.type == JSON_STRING) {
			identifier = appendee->value.string;
		} else {
			appendee->identifier = identifier;
			json_node_append(jobject, appendee);
			identifier = NULL;
		}
	}
	if (!_bufexpect('}', buffer, length, offset)) {
		json_node_free(jobject);
		return NULL;
	}
	return jobject;
}

static JsonNode* _array(char* buffer, ptrdiff_t length, ptrdiff_t* offset) {
	if (!_bufexpect('[', buffer, length, offset)) return NULL;
	JsonNode* jarray = json_node_create(NULL, (JsonValue){0, JSON_ARRAY});
	char nextChar;
	while ((nextChar = _bufpeek(buffer, length, *offset)) != ']' && *offset < length) {
		parserFunc currentParser = _getParser(nextChar);
		JsonNode* appendee = currentParser(buffer, length, offset);
		if (!appendee) {
			json_node_free(jarray);
			json_node_free(appendee);
			return NULL; // TODO: better error reporting
		} else if (appendee->value.type == JSON_INVALID) {
			json_node_free(appendee);
			continue;
		}
		json_node_append(jarray, appendee);
	}
	if (!_bufexpect(']', buffer, length, offset)) {
		json_node_free(jarray);
		return NULL;
	}
	return jarray;
}

static JsonNode* _boolean(char* buffer, ptrdiff_t length, ptrdiff_t* offset) {
	char* boolString = _scanWhile(_letterPredicate, buffer, length, offset);
	JsonNode* jnode;
	if (strcmp(boolString, "true") == 0) {
		jnode = json_node_create(NULL, (JsonValue){JSON_BOOL, .boolean = true});
	} else if (strcmp(boolString, "false") == 0) {
		jnode = json_node_create(NULL, (JsonValue){JSON_BOOL, .boolean = false});
	} else {
		jnode = NULL;
	}
	json_allocator.free(boolString, strlen(boolString), json_allocator.context);
	return jnode;
}

static JsonNode* _string(char* buffer, ptrdiff_t length, ptrdiff_t* offset) {
	_bufget(buffer, length, offset); // Not _bufexpect because at this point we know it's '"'
	char* string = _scanUntil("\"", buffer, length, offset);
	if (!string) return NULL;
	_bufget(buffer, length, offset);
	return json_node_create(NULL, (JsonValue){JSON_STRING, .string = string});
}

static JsonNode* _number(char* buffer, ptrdiff_t length, ptrdiff_t* offset) {
	char* numString = _scanWhile(_numberPredicate, buffer, length, offset);
	if (!numString) return NULL;
	char* end;
	double real = strtod(numString, &end);
	*offset = end - buffer; // set offset to last non number character.
	json_allocator.free(numString, strlen(numString), json_allocator.context);
	if (floor(real) == real) { // TODO: make sure this actually works (floating-point weirdness)
		return json_node_create(NULL, (JsonValue){JSON_INT, .integer = (int)real});
	}
	return json_node_create(NULL, (JsonValue){JSON_REAL, .real = real});
}

static JsonNode* _null(char* buffer, ptrdiff_t length, ptrdiff_t* offset) {
	char* nullString = _scanWhile(_letterPredicate, buffer, length, offset);
	if (!nullString) return NULL;
	JsonNode* jnode = NULL;
	if (strcmp(nullString, "null") == 0) {
		jnode = json_node_create(NULL, (JsonValue){JSON_NULL, 0});
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
			}
			return _skip;
	}
}

static inline bool _bufexpect(char c, char* buffer, ptrdiff_t length, ptrdiff_t* offset) {
	return c == _bufget(buffer, length, offset);
}

static inline char _bufget(char* buffer, ptrdiff_t length, ptrdiff_t* offset) {
	return *offset < length ? buffer[(*offset)++] : buffer[length - 1];
}

// TODO: It isn't clear enough from the return value when this function fails.
static inline char _bufput(char character, char* buffer, ptrdiff_t length, ptrdiff_t* offset) {
	return offset - 1 >= 0 ? buffer[--(*offset)] = character : 0;
}

static inline char _bufpeek(char* buffer, ptrdiff_t length, ptrdiff_t offset) {
	return offset < length ? buffer[offset] : buffer[length - 1];
}

char* _scanUntil(char* delimiters, char* buffer, ptrdiff_t length, ptrdiff_t* offset) {
	if (!delimiters || !buffer || !offset) return NULL;
	ptrdiff_t max = 16;
	ptrdiff_t current = 0;
	char* string = json_allocator.alloc(max, json_allocator.context);
	if (!string) return NULL;
	char currentChar;
	while (*offset < length && !strchr(delimiters, currentChar = _bufget(buffer, length, offset))) {
		if (current >= max - 2) { // minus 2 to leave room for null terminating character
			char* temp = json_allocator.realloc(string, max * 2, max, json_allocator.context);
			if (!temp) return NULL;
			string = temp;
			max *= 2;
		}
		string[current++] = currentChar;
	}
	string[current] = '\0';
	return string; 
}

char* _scanWhile(bool (*predicate)(char), char* buffer, ptrdiff_t length, ptrdiff_t* offset) {
	if (!predicate || !buffer || !offset) return NULL;
	ptrdiff_t max = 16;
	ptrdiff_t current = 0;
	char* string = json_allocator.alloc(max, json_allocator.context);
	if (!string) return NULL;
	char currentChar;
	while (*offset < length && predicate(currentChar = _bufget(buffer, length, offset))) {
		if (current >= max - 2) { // minus 2 to leave room for null terminating character
			char* temp = json_allocator.realloc(string, max * 2, max, json_allocator.context);
			if (!temp) return NULL;
			string = temp;
			max *= 2;
		}
		string[current++] = currentChar;
	}
	string[current] = '\0';
	return string;
}
