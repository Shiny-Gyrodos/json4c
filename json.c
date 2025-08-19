#include <stdint.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "json.h"

#define PRIVATE_DEFINITIONS
// Parsers
typedef JsonNode* (*parserFunc)(FILE*);
typedef JsonNode* parser(FILE* jstream);
static parser _skip;
static parser _object;
static parser _array;
static parser _boolean;
static parser _string;
static parser _number;
static parser _null;

// Helpers
static parserFunc _getParser(int);
static int _fpeek(FILE*);
static char* _scanUntil(FILE*, char*);
static char* _scanWhile(FILE*, bool (*)(int));
#undef PRIVATE_DEFINITIONS

#define PUBLIC_IMPLEMENTATIONS
JsonNode* jnode_create(JsonValue value) {
	JsonNode* jnode = malloc(sizeof(JsonNode));
	if (!jnode) return NULL;
	jnode->parent = NULL;
	jnode->identifier = NULL;
	jnode->value = value;
	if (IS_COMPLEX(value.type)) {
		puts("( jcomplex ) parsed");
		jnode->value.jcomplex.nodes = calloc(16, sizeof(JsonNode*));
		jnode->value.jcomplex.max = 16; // TODO: remove numeric literals
		jnode->value.jcomplex.count = 0;
	}
	return jnode;
}

void jnode_append(JsonNode* parent, JsonNode* child) {
	if (!parent || !child || !IS_COMPLEX(parent->value.type)) return;
	if (parent->value.jcomplex.count >= parent->value.jcomplex.max) {
		parent->value.jcomplex.max *= 2;
		void* temp = realloc(parent->value.jcomplex.nodes, sizeof(JsonNode*) * parent->value.jcomplex.max);
		if (!temp) return;
		parent->value.jcomplex.nodes = temp;
	}
	parent->value.jcomplex.nodes[parent->value.jcomplex.count] = child;
	parent->value.jcomplex.count++;
}

void jnode_free(JsonNode* jnode) {
	if (IS_COMPLEX(jnode->value.type)) {
		int i;
		for (i = 0; i < jnode->value.jcomplex.count; i++) {
			jnode_free(jnode->value.jcomplex.nodes[i]);
		}
	} else if (jnode->value.type == JSON_STRING) {
		free(jnode->value.string);
	}
	free(jnode->identifier);
	free(jnode);
}


JsonNode* json_parseFile(char* path) {
	const char* READ = "r";
	FILE* jstream = fopen(path, READ);
	int firstChar = _fpeek(jstream);
	if (firstChar == '\0') return NULL;
	parserFunc firstParser = _getParser(firstChar);
	if (!firstParser) return NULL;
	JsonNode* node = firstParser(jstream);
	return node;
}

bool json_isValid(char* path) {
	return json_parseFile(path) == NULL;
}

// TODO: Better error handling
JsonNode* json_get(JsonNode* root, size_t count, ...) {
	va_list args;
	va_start(args, count);
	while (count --> 0) { // Slide down-to operator :D 
		if (!root) return NULL;
		if (root->value.type == JSON_OBJECT) {
			char* string = va_arg(args, char*);
			int i = 0;
			while (true) {
				if (i >= root->value.jcomplex.count) {
					return NULL;
				}
				JsonNode* node = root->value.jcomplex.nodes[i];
				if (!node) {
					return NULL;
				}
				if (strcmp(string, node->identifier) == 0) {
					root = node;
					break;
				}
				i++;
			}
		} else if (root->value.type == JSON_ARRAY) {
			int index = va_arg(args, int);
			root = root->value.jcomplex.nodes[index];
		} else {
			// TODO: Handle error
		}
	}
	va_end(args);
	return root;
}
#undef PUBLIC_IMPLEMENTATIONS

#define PRIVATE_IMPLEMENTATIONS
// Predicates
static bool _numberPredicate(int character) { return isdigit(character); }
static bool _letterPredicate(int character) { return isalpha(character); }

static JsonNode* _skip(FILE* jstream) {
	puts("entered _skip");
	fgetc(jstream);
	puts("exited _skip");
	return jnode_create((JsonValue){JSON_INVALID, 0});
}

static JsonNode* _object(FILE* jstream) {
	puts("entered _object");
	fgetc(jstream); // Consume the '{'
	JsonNode* jnode = jnode_create((JsonValue){JSON_OBJECT, 0});
	char* identifier = NULL;
	int nextChar;
	while ((nextChar = _fpeek(jstream)) != '}') {
		parserFunc currentParser = _getParser(nextChar);
		JsonNode* appendee = currentParser(jstream);
		if (appendee->value.type == JSON_INVALID) {
			free(appendee);
			continue;
		} else if (!identifier && appendee->value.type == JSON_STRING) {
			// We have an identifier!
			identifier = appendee->value.string;
			printf("identifier = %s\n", identifier);
		} else {
			appendee->parent = jnode;
			appendee->identifier = identifier;
			jnode_append(jnode, appendee);
			identifier = NULL;
		}
	}
	fgetc(jstream); // Consume the '}'
	puts("exited _object");
	return jnode;
}

static JsonNode* _array(FILE* jstream) {
	puts("entered _array");
	fgetc(jstream); // Consume the '['
	JsonNode* jnode = jnode_create((JsonValue){JSON_ARRAY, 0});
	int nextChar;
	while ((nextChar = _fpeek(jstream)) != ']') {
		parserFunc currentParser = _getParser(nextChar);
		JsonNode* appendee = currentParser(jstream);
		if (appendee->value.type == JSON_INVALID) {
			free(appendee);
			continue;
		}
		appendee->parent = jnode;
		jnode_append(jnode, appendee);
	}
	fgetc(jstream); // Consume the ']'
	puts("exited _array");
	return jnode;
}

static JsonNode* _boolean(FILE* jstream) {
	puts("entered _boolean");
	char* boolString = _scanWhile(jstream, _letterPredicate);
	JsonNode* jnode;
	if (strcmp(boolString, "true") == 0) {
		puts("( true ) parsed");
		jnode = jnode_create((JsonValue){JSON_BOOL, .boolean = true});
	} else if (strcmp(boolString, "false") == 0) {
		puts("( false ) parsed");
		jnode = jnode_create((JsonValue){JSON_BOOL, .boolean = false});
	} else {
		// TODO: handle error
	}
	puts("exited _boolean");
	return jnode;
}

static JsonNode* _string(FILE* jstream) {
	puts("entered _string");
	fgetc(jstream); // Consume the '"'
	char* string = _scanUntil(jstream, "\"");
	fgetc(jstream); // Consume the other '"'
	printf("( %s ) parsed\n", string);
	puts("exited _string");
	return jnode_create((JsonValue){JSON_STRING, .string = string});
}

static JsonNode* _number(FILE* jstream) {
	puts("entered _number");
	char* numberString = _scanWhile(jstream, _numberPredicate);
	if (_fpeek(jstream) == '.') {
		char* appendee = _scanUntil(jstream, ", \t\n}]");
		numberString = strcat(numberString, appendee);
		float real = (float)atof(numberString);
		printf("( %f ) parsed\n", real);
		puts("exited _number");
		return jnode_create((JsonValue){JSON_REAL, .real = real});
	}
	int integer = atoi(numberString);
	printf("( %d ) parsed\n", integer);
	puts("exited _number");
	return jnode_create((JsonValue){JSON_INT, .integer = integer});
}

static JsonNode* _null(FILE* jstream) {
	puts("entered _null");
	char* nullString = _scanWhile(jstream, _letterPredicate);
	JsonNode* jnode;
	if (strcmp(nullString, "null") == 0) {
		puts("( null ) parsed");
		jnode = jnode_create((JsonValue){JSON_NULL, 0});
	} else {
		// TODO: handle error
	}
	puts("exited _null");
	return jnode;
}

static parserFunc _getParser(int character) {
	if (character == EOF) return NULL;
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
		default: // number OR whitespace
			if (isdigit(character)) {
				return _number;
			}
			return _skip;
	}
}

static int _fpeek(FILE* stream) {
	int character = fgetc(stream);
	return character == EOF ? EOF : ungetc(character, stream);
}

static char* _scanUntil(FILE* stream, char* delimiters) {
	if (!stream || !delimiters) return NULL;
	char* string = malloc(8);
	if (!string) return NULL;
	size_t max = 8;
	size_t count = 0;
	int nextChar;
	while ((nextChar = fgetc(stream)) != EOF) {
		if (count >= max) {
			max *= 2;
			char* temp = realloc(string, max);
			if (!temp) {
				free(string);
				return NULL;
			}
			string = temp;
		}
		int i;
		for (i = 0; delimiters[i] != '\0'; i++) {
			if (nextChar == delimiters[i]) {
				ungetc(nextChar, stream);
				string[count] = '\0';
				return string;
			}
		}
		string[count++] = nextChar;
	}
	free(string);
	return NULL;
}

static char* _scanWhile(FILE* stream, bool (predicate)(int)) {
	if (!stream || !predicate) return NULL;
	char* string = malloc(8);
	if (!string) return NULL;
	size_t max = 8;
	size_t count = 0;
	int nextChar;
	while ((nextChar = fgetc(stream)) != EOF) {
		if (count >= max) {
			max *= 2;
			char* temp = realloc(string, max);
			if (!temp) {
				free(string);
				return NULL;
			}
			string = temp;
		}
		if (!predicate(nextChar)) {
			ungetc(nextChar, stream);
			string[count] = '\0';
			return string;
		}
		string[count++] = nextChar;
	}
	free(string);
	return NULL;
}
#undef PRIVATE_IMPLEMENTATIONS
