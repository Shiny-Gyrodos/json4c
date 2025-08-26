#include <stdint.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "json.h"

#undef DEBUG
#ifdef JSON4C_DEBUG
#define DEBUG(msg) printf("[%s:%d] %s\n", __FILE__, __LINE__, msg)
#else
#define DEBUG(msg)
#endif

static void* (*json_alloc)(size_t) = JSON_DEFAULT_ALLOC;
static void (*json_free)(void*) = JSON_DEFAULT_FREE;

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
static void _serialize(FILE* jsonFile, JsonNode* jnode, char* indent, char* extra);

// Helpers
static parserFunc _getParser(int);
static int _fpeek(FILE*);
static char* _strcombine(const char*, const char*);
static char* _scanUntil(FILE*, char*);
static char* _scanWhile(FILE*, bool (*)(int));
#undef PRIVATE_DEFINITIONS

#define PUBLIC_IMPLEMENTATIONS
JsonNode* jnode_create(char* identifier, JsonValue value) {
	JsonNode* jnode = json_alloc(sizeof(JsonNode));
	if (!jnode) return NULL;
	jnode->identifier = identifier;
	jnode->value = value;
	if (IS_COMPLEX(value.type)) {
		jnode->value.jcomplex.nodes = json_alloc(sizeof(JsonNode*) * JSON_COMPLEX_DEFAULT_CAPACITY);
		memset(jnode->value.jcomplex.nodes, 0, sizeof(JsonNode*) * JSON_COMPLEX_DEFAULT_CAPACITY);
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
		json_free(jnode->value.string);
	}
	json_free(jnode->identifier);
	json_free(jnode);
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
	_serialize(jsonFile, jnode, indent, "");
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


void json_setAllocator(void* (*custom_malloc)(size_t), void (*custom_free)(void*)) {
	if (!custom_malloc || !custom_free) return;
	json_alloc = custom_malloc;
	json_free = custom_free;
}

void json_resetAllocator(void) {
	json_alloc = malloc;
	json_free = free;
}
#undef PUBLIC_IMPLEMENTATIONS

#define PRIVATE_IMPLEMENTATIONS
// Predicates
static bool _intPredicate(int character) { return isdigit(character); }
static bool _realPredicate(int character) { return character == '.' || isdigit(character); }
static bool _letterPredicate(int character) { return isalpha(character); }

static JsonNode* _skip(FILE* jstream) {
	DEBUG("entered _skip");
	fgetc(jstream);
	DEBUG("exited _skip");
	return jnode_create(NULL, (JsonValue){JSON_INVALID, 0});
}

static JsonNode* _object(FILE* jstream) {
	DEBUG("entered _object");
	fgetc(jstream); // Consume the '{'
	DEBUG("( { ) parsed");
	JsonNode* jnode = jnode_create(NULL, (JsonValue){JSON_OBJECT, 0});
	char* identifier = NULL;
	int nextChar;
	while ((nextChar = _fpeek(jstream)) != '}' && !feof(jstream)) {
		parserFunc currentParser = _getParser(nextChar);
		JsonNode* appendee = currentParser(jstream);
		if (!appendee) { // Parsing failed
			jnode_free(jnode);
			DEBUG("parser returned NULL, bailing out");
			return NULL;
		} else if (appendee->value.type == JSON_INVALID) {
			json_free(appendee);
			continue;
		} else if (!identifier && appendee->value.type == JSON_STRING) {
			// We have an identifier!
			identifier = appendee->value.string;
			#ifdef JSON4C_DEBUG
			printf("[%s:%d] ( %s ) identifier parsed\n", __FILE__, __LINE__, identifier);
			#endif
		} else {
			appendee->identifier = identifier;
			jnode_append(jnode, appendee);
			identifier = NULL;
		}
	}
	// Consume the '}'
	if (fgetc(jstream) != '}') {
		jnode_free(jnode);
		DEBUG("( } ) missing, bailing out");
		return NULL;
	}
	DEBUG("( } ) parsed");
	DEBUG("exited _object");
	return jnode;
}

static JsonNode* _array(FILE* jstream) {
	DEBUG("entered _array");
	fgetc(jstream); // Consume the '['
	DEBUG("( [ ) parsed");
	JsonNode* jnode = jnode_create(NULL, (JsonValue){JSON_ARRAY, 0});
	int nextChar;
	while ((nextChar = _fpeek(jstream)) != ']' && !feof(jstream)) {
		parserFunc currentParser = _getParser(nextChar);
		JsonNode* appendee = currentParser(jstream);
		if (!appendee) { // Parsing failed
			jnode_free(jnode);
			DEBUG("parser returned NULL, bailing out");
			return NULL;
		} else if (appendee->value.type == JSON_INVALID) {
			json_free(appendee);
			continue;
		}
		jnode_append(jnode, appendee);
	}
	// Consume the ']'
	if (fgetc(jstream) != ']') {
		jnode_free(jnode);
		DEBUG("( ] ) missing, bailing out");
		return NULL;
	}
	DEBUG("( ] ) parsed");
	DEBUG("exited _array");
	return jnode;
}

static JsonNode* _boolean(FILE* jstream) {
	DEBUG("entered _boolean");
	char* boolString = _scanWhile(jstream, _letterPredicate);
	JsonNode* jnode;
	if (strcmp(boolString, "true") == 0) {
		DEBUG("( true ) parsed");
		jnode = jnode_create(NULL, (JsonValue){JSON_BOOL, .boolean = true});
	} else if (strcmp(boolString, "false") == 0) {
		DEBUG("( false ) parsed");
		jnode = jnode_create(NULL, (JsonValue){JSON_BOOL, .boolean = false});
	} else {
		DEBUG("_boolean failed to parse");
		jnode = NULL;
	}
	json_free(boolString);
	DEBUG("exited _boolean");
	return jnode;
}

static JsonNode* _string(FILE* jstream) {
	DEBUG("entered _string");
	fgetc(jstream); // Consume the '"'
	char* string = _scanUntil(jstream, "\"");
	if (!string) {
		DEBUG("_string failed to parse");
		DEBUG("exited _string");
		return NULL;
	}
	fgetc(jstream); // Consume the other '"'
	#ifdef JSON4C_DEBUG
	printf("[%s:%d] ( %s ) parsed\n", __FILE__, __LINE__, string);
	#endif
	DEBUG("exited _string");
	return jnode_create(NULL, (JsonValue){JSON_STRING, .string = string});
}

static JsonNode* _number(FILE* jstream) {
	DEBUG("entered _number");
	char* numberString = _scanWhile(jstream, _intPredicate);
	if (_fpeek(jstream) == '.') {
		char* appendee = _scanWhile(jstream, _realPredicate);
		numberString = strcat(numberString, appendee);
		float real = (float)atof(numberString);
		json_free(appendee);
		json_free(numberString);
		#ifdef JSON4C_DEBUG
		printf("[%s:%d] ( %f ) parsed\n", __FILE__, __LINE__, real);
		#endif
		DEBUG("exited _number");
		return jnode_create(NULL, (JsonValue){JSON_REAL, .real = real});
	}
	int integer = atoi(numberString);
	json_free(numberString);
	#ifdef JSON4C_DEBUG
	printf("[%s:%d] ( %d ) parsed\n", __FILE__, __LINE__, integer);
	#endif
	DEBUG("exited _number");
	return jnode_create(NULL, (JsonValue){JSON_INT, .integer = integer});
}

static JsonNode* _null(FILE* jstream) {
	DEBUG("entered _null");
	char* nullString = _scanWhile(jstream, _letterPredicate);
	JsonNode* jnode;
	if (nullString && strcmp(nullString, "null") == 0) {
		DEBUG("( null ) parsed");
		jnode = jnode_create(NULL, (JsonValue){JSON_NULL, 0});
	} else {
		DEBUG("_null failed to parse");
		jnode = NULL;
	}
	json_free(nullString);
	DEBUG("exited _null");
	return jnode;
}

// TODO: needs a serious refactor
static void _serialize(FILE* jsonFile, JsonNode* jnode, char* indent, char* extra) {
	if (!jsonFile || !jnode) return;
	if (!indent) {
		indent = "\t";
	}
	char* firstIndent = indent;
	if (jnode->identifier) {
		firstIndent = "";
		fprintf(jsonFile, "%s\"%s\": ", indent, jnode->identifier);
	}
	switch (jnode->value.type) {
		case JSON_OBJECT:
		case JSON_ARRAY: {
			// For both '{' and '[', adding two ('{' + 2) gives their closing counterpart.
			char open = jnode->value.type == JSON_OBJECT ? '{' : '[';
			char close = open + 2;
			fprintf(jsonFile, "%s%c\n", firstIndent, open);
			int i;
			for (i = 0; i < jnode->value.jcomplex.count; i++) {
				char* newIndent = _strcombine(indent, "\t");
				_serialize(
					jsonFile, 
					jnode->value.jcomplex.nodes[i], 
					newIndent, 
					jnode->value.jcomplex.nodes[i + 1] == NULL ? "\n" : ",\n"
				);
				json_free(newIndent); // _strcombine return value is malloc'ed
			}
			fprintf(jsonFile, "%s%c%s", indent, close, extra);
			break;
		}
		case JSON_BOOL:
			fprintf(jsonFile, "%s%s%s", firstIndent, AS_BOOL(jnode) ? "true" : "false", extra);
			break;
		case JSON_STRING:
			fprintf(jsonFile, "%s\"%s\"%s", firstIndent, AS_STRING(jnode), extra);
			break;
		case JSON_NULL:
			fprintf(jsonFile, "%snull%s", firstIndent, extra);
			break;
		case JSON_INT:
			fprintf(jsonFile, "%s%d%s", firstIndent, AS_INT(jnode), extra);
			break;
		case JSON_REAL:
			fprintf(jsonFile, "%s%f%s", firstIndent, AS_REAL(jnode), extra);
			break;
		default:
			break;
	}
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
	char* string = json_alloc(strlen(s1) + strlen(s2) + 1);
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
	char* string = json_alloc(8);
	if (!string) return NULL;
	size_t max = 8;
	size_t count = 0;
	int nextChar;
	while ((nextChar = fgetc(stream)) != EOF) {
		if (count >= max) {
			max *= 2;
			char* temp = realloc(string, max);
			if (!temp) {
				json_free(string);
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
	json_free(string);
	return NULL;
}

static char* _scanWhile(FILE* stream, bool (predicate)(int)) {
	if (!stream || !predicate) return NULL;
	char* string = json_alloc(8);
	if (!string) return NULL;
	size_t max = 8;
	size_t count = 0;
	int nextChar;
	while ((nextChar = fgetc(stream)) != EOF) {
		if (count >= max) {
			max *= 2;
			char* temp = realloc(string, max);
			if (!temp) {
				json_free(string);
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
	json_free(string);
	return NULL;
}
#undef PRIVATE_IMPLEMENTATIONS
