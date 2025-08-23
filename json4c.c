#include <stdint.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "json4c.h"

#define PRIVATE_DEFINITIONS
// Parsers
typedef JsonNode* (*parserFunc)(FILE*);
typedef JsonNode* parser(FILE*);
static parser _skip;
static parser _object;
static parser _array;
static parser _boolean;
static parser _string;
static parser _number;
static parser _null;

// Serialization
static void _serialize(FILE* jsonFile, JsonNode* jnode, char* indent);

// Helpers
static parserFunc _getParser(int);
static int _fpeek(FILE*);
static char* _strcombine(const char*, const char*);
static char* _scanUntil(FILE*, char*);
static char* _scanWhile(FILE*, bool (*)(int));
#undef PRIVATE_DEFINITIONS

#define PUBLIC_IMPLEMENTATIONS
JsonNode* jnode_create(char* identifier, JsonValue value) {
	JsonNode* jnode = malloc(sizeof(JsonNode));
	if (!jnode) return NULL;
	jnode->identifier = identifier;
	jnode->value = value;
	if (IS_COMPLEX(value.type)) {
		puts("( jcomplex ) parsed");
		jnode->value.jcomplex.nodes = calloc(JSON_COMPLEX_DEFAULT_CAPACITY, sizeof(JsonNode*));
		jnode->value.jcomplex.max = JSON_COMPLEX_DEFAULT_CAPACITY;
		jnode->value.jcomplex.count = 0;
	}
	return jnode;
}

void jnode_append(JsonNode* parent, JsonNode* child) {
	if (!parent || !child || !IS_COMPLEX(parent->value.type)) return;
	if (parent->value.jcomplex.count >= parent->value.jcomplex.max) {
		parent->value.jcomplex.max *= JSON_COMPLEX_GROW_MULTIPLIER;
		void* temp = realloc(parent->value.jcomplex.nodes, sizeof(JsonNode*) * parent->value.jcomplex.max);
		if (!temp) return;
		parent->value.jcomplex.nodes = temp;
	}
	parent->value.jcomplex.nodes[parent->value.jcomplex.count] = child;
	parent->value.jcomplex.count++;
}

void jnode_free(JsonNode* jnode) {
	if (!jnode) return;
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


JsonNode* _json_object(JsonNode** jnodes) {
	JsonNode* jobject = jnode_create(NULL, (JsonValue){JSON_OBJECT, 0});
	int i;
	char* identifier = NULL;
	for (i = 0; jnodes[i]; i++) {
		if (i % 2 == 0) {
			identifier = jnodes[i]->value.string;
		} else {
			jnodes[i]->identifier = identifier;
			identifier = NULL;
			jnode_append(jobject, jnodes[i]);
		}
	}
	return jobject;
}

JsonNode* _json_array(JsonNode** jnodes) {
	JsonNode* jarray = jnode_create(NULL, (JsonValue){JSON_ARRAY, 0});
	int i;
	for (i = 0; jnodes[i]; i++) {
		jnode_append(jarray, jnodes[i]);
	}
	return jarray;
}

inline JsonNode* json_bool(bool boolean) {
	return jnode_create(NULL, (JsonValue){JSON_BOOL, .boolean = boolean});
}

inline JsonNode* json_int(int integer) {
	return jnode_create(NULL, (JsonValue){JSON_INT, .integer = integer});
}

inline JsonNode* json_real(float real) {
	return jnode_create(NULL, (JsonValue){JSON_REAL, .real = real});
}

inline JsonNode* json_null(void) {
	return jnode_create(NULL, (JsonValue){JSON_NULL, 0});
}

inline JsonNode* json_string(char* string) {
	return jnode_create(NULL, (JsonValue){JSON_STRING, .string = string});
}

bool json_write(char* path, JsonNode* jnode, char* indent) {
	const char* WRITE = "w";
	FILE* jsonFile = fopen(path, WRITE);
	if (!jsonFile) return false;
	_serialize(jsonFile, jnode, indent);
	fclose(jsonFile);
	return true;
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
			return NULL;
		}
	}
	va_end(args);
	return root;
}
#undef PUBLIC_IMPLEMENTATIONS

#define PRIVATE_IMPLEMENTATIONS
// Predicates
static bool _intPredicate(int character) { return isdigit(character); }
static bool _realPredicate(int character) { return character == '.' || isdigit(character); }
static bool _letterPredicate(int character) { return isalpha(character); }

static JsonNode* _skip(FILE* jstream) {
	puts("entered _skip");
	fgetc(jstream);
	puts("exited _skip");
	return jnode_create(NULL, (JsonValue){JSON_INVALID, 0});
}

static JsonNode* _object(FILE* jstream) {
	puts("entered _object");
	fgetc(jstream); // Consume the '{'
	JsonNode* jnode = jnode_create(NULL, (JsonValue){JSON_OBJECT, 0});
	char* identifier = NULL;
	int nextChar;
	while ((nextChar = _fpeek(jstream)) != '}' && !feof(jstream)) {
		parserFunc currentParser = _getParser(nextChar);
		JsonNode* appendee = currentParser(jstream);
		if (!appendee) { // Parsing failed
			free(jnode);
			return NULL;
		} else if (appendee->value.type == JSON_INVALID) {
			free(appendee);
			continue;
		} else if (!identifier && appendee->value.type == JSON_STRING) {
			// We have an identifier!
			identifier = appendee->value.string;
			printf("identifier = %s\n", identifier);
		} else {
			appendee->identifier = identifier;
			jnode_append(jnode, appendee);
			identifier = NULL;
		}
	}
	// Consume the '}'
	if (fgetc(jstream) != '}') return NULL;
	puts("exited _object");
	return jnode;
}

static JsonNode* _array(FILE* jstream) {
	puts("entered _array");
	fgetc(jstream); // Consume the '['
	JsonNode* jnode = jnode_create(NULL, (JsonValue){JSON_ARRAY, 0});
	int nextChar;
	while ((nextChar = _fpeek(jstream)) != ']' && !feof(jstream)) {
		parserFunc currentParser = _getParser(nextChar);
		JsonNode* appendee = currentParser(jstream);
		if (!appendee) { // Parsing failed
			free(jnode);
			return NULL;
		} else if (appendee->value.type == JSON_INVALID) {
			free(appendee);
			continue;
		}
		jnode_append(jnode, appendee);
	}
	// Consume the ']'
	if (fgetc(jstream) != ']') return NULL;
	puts("exited _array");
	return jnode;
}

static JsonNode* _boolean(FILE* jstream) {
	puts("entered _boolean");
	char* boolString = _scanWhile(jstream, _letterPredicate);
	JsonNode* jnode;
	if (strcmp(boolString, "true") == 0) {
		puts("( true ) parsed");
		jnode = jnode_create(NULL, (JsonValue){JSON_BOOL, .boolean = true});
	} else if (strcmp(boolString, "false") == 0) {
		puts("( false ) parsed");
		jnode = jnode_create(NULL, (JsonValue){JSON_BOOL, .boolean = false});
	} else {
		jnode = NULL;
	}
	free(boolString);
	puts("exited _boolean");
	return jnode;
}

static JsonNode* _string(FILE* jstream) {
	puts("entered _string");
	fgetc(jstream); // Consume the '"'
	char* string = _scanUntil(jstream, "\"");
	if (!string) {
		puts("exited _string");
		return NULL;
	}
	fgetc(jstream); // Consume the other '"'
	printf("( %s ) parsed\n", string);
	puts("exited _string");
	return jnode_create(NULL, (JsonValue){JSON_STRING, .string = string});
}

static JsonNode* _number(FILE* jstream) {
	puts("entered _number");
	char* numberString = _scanWhile(jstream, _intPredicate);
	if (_fpeek(jstream) == '.') {
		char* appendee = _scanWhile(jstream, _realPredicate);
		numberString = strcat(numberString, appendee);
		float real = (float)atof(numberString);
		free(appendee);
		free(numberString);
		printf("( %f ) parsed\n", real);
		puts("exited _number");
		return jnode_create(NULL, (JsonValue){JSON_REAL, .real = real});
	}
	int integer = atoi(numberString);
	free(numberString);
	printf("( %d ) parsed\n", integer);
	puts("exited _number");
	return jnode_create(NULL, (JsonValue){JSON_INT, .integer = integer});
}

static JsonNode* _null(FILE* jstream) {
	puts("entered _null");
	char* nullString = _scanWhile(jstream, _letterPredicate);
	JsonNode* jnode;
	if (nullString && strcmp(nullString, "null") == 0) {
		puts("( null ) parsed");
		jnode = jnode_create(NULL, (JsonValue){JSON_NULL, 0});
	} else {
		jnode = NULL;
	}
	free(nullString);
	puts("exited _null");
	return jnode;
}

// TODO: needs a serious refactor
// TODO: only add trailing commas when necessary
static void _serialize(FILE* jsonFile, JsonNode* jnode, char* indent) {
#define HAS_IDENTIFIER ((jnode)->identifier)
	switch (jnode->value.type) {
		case JSON_OBJECT:
		case JSON_ARRAY: {
			// For both '{' and '[', adding two ('{' + 2) gives their closing counterpart.
			char open = jnode->value.type == JSON_OBJECT ? '{' : '[';
			char close = open + 2;
			if (HAS_IDENTIFIER) {
				fprintf(jsonFile, "%s\"%s\": %c\n", indent, jnode->identifier, open);
			} else {
				fprintf(jsonFile, "%s%c\n", indent, open);
			}
			int i;
			for (i = 0; i < jnode->value.jcomplex.count; i++) {
				char* newIndent = _strcombine(indent, "\t");
				_serialize(jsonFile, jnode->value.jcomplex.nodes[i], _strcombine(indent, "\t"));
				free(newIndent); // _strcombine return value is malloc'ed
			}
			fprintf(jsonFile, "%s%c", indent, close);
			break;
		}
		case JSON_BOOL:
			if (HAS_IDENTIFIER) {
				fprintf(
					jsonFile, 
					"%s\"%s\": %s", 
					indent, 
					jnode->identifier, 
					jnode->value.boolean ? "true" : "false"
				);
			} else {
				fprintf(jsonFile, "%s%s", indent, jnode->value.boolean ? "true" : "false");
			}
			break;
		case JSON_STRING:
			if (HAS_IDENTIFIER) {
				fprintf(
					jsonFile,
					"%s\"%s\": \"%s\"",
					indent,
					jnode->identifier,
					jnode->value.string
				);
			} else {
				fprintf(jsonFile, "%s\"%s\"", indent, jnode->value.string);
			}
			break;
		case JSON_NULL:
			if (HAS_IDENTIFIER) {
				fprintf(jsonFile, "%s\"%s\": null", indent, jnode->identifier);
			} else {
				fprintf(jsonFile, "%snull", indent);
			}
			break;
		case JSON_INT:
			if (HAS_IDENTIFIER) {
				fprintf(jsonFile, "%s\"%s\": %d", indent, jnode->identifier, jnode->value.integer);
			} else {
				fprintf(jsonFile, "%s%d", indent, jnode->value.integer);
			}
			break;
		case JSON_REAL:
			if (HAS_IDENTIFIER) {
				fprintf(jsonFile, "%s\"%s\": %f", indent, jnode->identifier, jnode->value.real);
			} else {
				fprintf(jsonFile, "%s%f", indent, jnode->value.real);
			}
			break;
		default:
			return;
	}
	fputs(",\n", jsonFile);
#undef HAS_IDENTIFIER
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
		default: // number or whitespace
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

static char* _strcombine(const char* s1, const char* s2) {
	// + 1 for '\0'
	char* string = malloc(strlen(s1) + strlen(s2) + 1);
	if (!string) return NULL;
	int i;
	for (i = 0; s1[i] != '\0'; i++) {
		string[i] = s1[i];
	}
	int j;
	for (j = 0; s2[j] != '\0'; i++, j++) {
		string[i] = s2[j];
	}
	string[i] = '\0';
	return string;
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
		if (strchr(delimiters, nextChar)) {
			ungetc(nextChar, stream);
			string[count] = '\0';
			return string;
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
