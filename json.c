#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h> 

#include "json.h"

JsonNode* jnode_create(JsonNode* parent, char* identifier, JsonType type) {
	JsonNode* node = malloc(sizeof(JsonNode));
	if (!node) return NULL;
	node->parent = parent;
	node->identifier = identifier;
	node->type = type;
	if (type & JSON_COMPLEX) {
		node->value.jsonComplex.nodes = calloc(16, sizeof(JsonNode*));
		if (!node->value.jsonComplex.nodes) return NULL;
		node->value.jsonComplex.max = 16;
		node->value.jsonComplex.count = 0;
	}
	return node;
}

// TODO: Should this function assign child->parent to parent?
void jnode_append(JsonNode* parent, JsonNode* child) {
	if (!parent || !child || !(parent->type & JSON_COMPLEX)) return;
	if (parent->value.jsonComplex.count >= parent->value.jsonComplex.max) {
		parent->value.jsonComplex.max *= 2;
		parent->value.jsonComplex.nodes = realloc(
			parent->value.jsonComplex.nodes, 
			parent->value.jsonComplex.max * sizeof(JsonNode*)
		);
		if (!parent->value.jsonComplex.nodes) return; // TODO: Add error message.
	}
	parent->value.jsonComplex.nodes[parent->value.jsonComplex.count] = child;
	parent->value.jsonComplex.count++;
	printf("count = %zu\n", parent->value.jsonComplex.count);
}

void jnode_free(JsonNode* node) {
	free(node->identifier);
	if (node->type & JSON_COMPLEX) {
		int i;
		for (i = 0; i < node->value.jsonComplex.count; i++) {
			jnode_free(node->value.jsonComplex.nodes[i]);
		}
	}
	free(node);
}


static int _fpeek(FILE* stream) {
	int nextChar = fgetc(stream);
	return nextChar == EOF ? EOF : ungetc(nextChar, stream);
}

static char* _scanUntil(FILE* stream, char* delimiters) {
	char* string = malloc(8);
	if (!string) return NULL;
	size_t max = 8;
	size_t count = 0;
	int nextChar = fgetc(stream);
	// Should handle encountering EOF somehow.
	while (nextChar != EOF) {
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
				printf("( %c ) pushed\n", nextChar);
				string[count] = '\0';
				return string;
			}
		}
		string[count] = nextChar;
		count++;
		printf("( %c ) parsed\n", nextChar);
		nextChar = fgetc(stream);
	}
	free(string);
	return NULL; // I guess this sort of indicates EOF was reached?
}

static int _nextNonWhitespace(FILE* stream) {
	int nextChar;
	do {
		nextChar = fgetc(stream);
	} while (nextChar != EOF && isspace(nextChar));
	if (nextChar == EOF) return EOF;
	return ungetc(nextChar, stream);
}

// TODO: It would make more sense for this function to return "union JsonValue"
static JsonNode* _parseValue(char* valueString) {
	size_t length = strlen(valueString);
	while (length > 0 && isspace(valueString[length - 1])) {
		length--;
	}
	char* string = malloc(length + 1); // Make room for '\0'
	memcpy(string, valueString, length);
	string[length] = '\0';
	
	JsonNode* node;
	if (strcmp("true", string) == 0) {
		puts("( true ) parsed");
		node = jnode_create(NULL, NULL, JSON_BOOL);
		node->value.boolean = true;
	} else if (strcmp("false", string) == 0) {
		puts("( false ) parsed");
		node = jnode_create(NULL, NULL, JSON_BOOL);
		node->value.boolean = false;
	} else if (strcmp("null", string) == 0) {
		puts("( null ) parsed");
		node = jnode_create(NULL, NULL, JSON_NULL);
	} else if (string[0] == '"' && string[length - 1] == '"') { // TODO: Fix sketchy check
		puts("( string ) parsed");
		node = jnode_create(NULL, NULL, JSON_STRING);
		node->value.string = malloc(length - 1);
		memcpy(node->value.string, string + 1, length - 1);
		node->value.string[length - 2] = '\0';
	} else if (strchr(string, '.')) {
		puts("( real ) parsed");
		node = jnode_create(NULL, NULL, JSON_REAL);
		node->value.real = atof(string);
	} else { // int
		puts("( integer ) parsed");
		node = jnode_create(NULL, NULL, JSON_INT);
		node->value.integer = atoi(string);
	}
	return node;
}

// TODO: Could use a serious refactor, there is a lot of duplicate code,
// TODO: and there is a lack of clarity on when a char is consumed,
// TODO: and whose responsibility it is to consume chars.
static JsonNode* _parseNext(FILE* jsonStream, JsonNode* parent) {
	JsonNode* node;
	char* identifier = NULL;
	
	int nextChar = _nextNonWhitespace(jsonStream);
	if (nextChar == EOF) return parent;
	printf("( %c ) parsed\n", (char)_fpeek(jsonStream));
	
	switch ((char)nextChar) {
		case ',':
		case ':': // TODO: Add syntax checking
			fgetc(jsonStream); // Consume useless character.
			return _parseNext(jsonStream, parent);
		case '{':
			fgetc(jsonStream);
			puts("object start parsed");
			node = jnode_create(parent, identifier, JSON_OBJECT);
			jnode_append(parent, node);
			return _parseNext(jsonStream, node);
		case '[':
			fgetc(jsonStream);
			puts("array start parsed");
			node = jnode_create(parent, identifier, JSON_ARRAY);
			jnode_append(parent, node);
			return _parseNext(jsonStream, node);
		case '}':
		case ']':
			fgetc(jsonStream);
			puts("object or array end parsed");
			return parent->parent == NULL ?
				   parent :
				   _parseNext(jsonStream, parent->parent);
		case '"': {
			fgetc(jsonStream); // Consume the '"'
			char* string = _scanUntil(jsonStream, "\"");
			fgetc(jsonStream); // Consume the '"'
			printf("( %c ) parsed\n", (char)_fpeek(jsonStream));
			if (_nextNonWhitespace(jsonStream) == ':') {
				fgetc(jsonStream); // Consume the ':'	
				puts("identifier parsed");
				nextChar = _nextNonWhitespace(jsonStream);
				printf("( %c ) parsed\n", (char)nextChar);
				if (nextChar == '{') {
					puts("object start parsed");
					fgetc(jsonStream); // Consume '{'
					node = jnode_create(parent, string, JSON_OBJECT);
					jnode_append(parent, node);
					return _parseNext(jsonStream, node);
				} else if (nextChar == '[') {
					puts("array start parsed");
					fgetc(jsonStream); // Consume '['
					node = jnode_create(parent, string, JSON_ARRAY);
					jnode_append(parent, node);
					return _parseNext(jsonStream, node);
				}
				identifier = string;
				// Drop through
			} else {
				// TODO: Remove sort of duplicate code
				puts("string literal parsed");
				if (_fpeek(jsonStream) == ',') {
					fgetc(jsonStream); // Consume the ','
				}
				node = jnode_create(parent, identifier, JSON_STRING);
				node->value.string = string;
				jnode_append(parent, node);
				return _parseNext(jsonStream, parent);
			}
		}
		default:
			puts("primitive parsed");
			char* valueString = _scanUntil(jsonStream, ",]}");
			printf("valueString = %s\n", valueString);
			if (_fpeek(jsonStream) == ',') {
				fgetc(jsonStream); // Consume the ','
			}
			node = _parseValue(valueString);
			node->identifier = identifier;
			node->parent = parent;
			jnode_append(parent, node);
			return _parseNext(jsonStream, parent);
	}
	return NULL;
}

JsonNode* json_parseFile(char* path) {
	const char* READ = "r";
	FILE* jsonStream = fopen(path, READ);
	JsonNode* node = _parseNext(jsonStream, NULL); // Start the parser.
	fclose(jsonStream);
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
		printf("root->type = %d\n", root->type);
		if (root->type == JSON_OBJECT) {
			char* string = va_arg(args, char*);
			int i = 0;
			while (true) {
				if (i >= root->value.jsonComplex.count) {
					puts("oh no");
					return NULL;
				}
				JsonNode* node = root->value.jsonComplex.nodes[i];
				if (!node) {
					puts("oh boy");
					return NULL;
				}
				printf("( %s ) identifier compared\n", node->identifier);
				if (strcmp(string, node->identifier) == 0) {
					root = node;
					break;
				}
				i++;
			}
		} else if (root->type == JSON_ARRAY) {
			int index = va_arg(args, int);
			root = root->value.jsonComplex.nodes[index];
		} else {
			// TODO: Handle error
		}
	}
	va_end(args);
	return root;
}
