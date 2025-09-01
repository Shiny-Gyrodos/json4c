#include "json_parser.h"

// TODO: All parser need to be rewritten to accomodate the new allocator.
// TODO: Number parser needs extra work to support numbers like -11.86e2
// TODO: String parser needs extra work to support escaped characters.

// Parsers
typedef JsonNode* (*parserFunc)(char*, ptrdiff_t, ptrdiff_t);
typedef JsonNode* parser(char*, ptrdiff_t, ptrdiff_t);
static parser _skip;
static parser _object;
static parser _array;
static parser _boolean;
static parser _string;
static parser _number;
static parser _null;

static parserFunc _getParser(int character);


JsonNode* json_parse(char* buffer, ptrdiff_t length, ptrdiff_t offset) {
	return NULL; // Placeholder
}

JsonNode* json_parseFile(char* path) {
	return NULL; // placeholder
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


// Predicates
static bool _intPredicate(int character) { return isdigit(character); }
static bool _realPredicate(int character) { // TODO: fix bandaid fix
	return character == '.' || character == 'e' || character == 'E' || isdigit(character); 
}
static bool _numberPrefixPredicate(int character) { 
	return character == '+' || character == '-' || isdigit(character); 
}
static bool _letterPredicate(int character) { return isalpha(character); }

static JsonNode* _skip(char* buffer, ptrdiff_t length, ptrdiff_t offset) {
	DEBUG("entered _skip");
	fgetc(jstream);
	DEBUG("exited _skip");
	return jnode_create(NULL, (JsonValue){JSON_INVALID, 0});
}

static JsonNode* _object(char* buffer, ptrdiff_t length, ptrdiff_t offset) {
	DEBUG("entered _object");
	if (!_expect('{', fgetc(jstream))) { // Consume the '{'
		DEBUG("object missing starting brace, bailing out");
		return NULL;
	}
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
			json_allocator.free(allocator.context, appendee, sizeof(JsonNode));
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
	if (!_expect('}', fgetc(jstream))) {
		jnode_free(jnode);
		DEBUG("( } ) missing, bailing out");
		return NULL;
	}
	DEBUG("( } ) parsed");
	DEBUG("exited _object");
	return jnode;
}

static JsonNode* _array(char* buffer, ptrdiff_t length, ptrdiff_t offset) {
	DEBUG("entered _array");
	if (!_expect('[', fgetc(jstream))) { // Consume the '['
		DEBUG("array missing starting bracket, bailing out");
		return NULL;
	}
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
			json_allocator.free(allocator.context, appendee, sizeof(JsonNode));
			continue;
		}
		jnode_append(jnode, appendee);
	}
	// Consume the ']'
	if (!_expect('[', fgetc(jstream))) {
		jnode_free(jnode);
		DEBUG("( ] ) missing, bailing out");
		return NULL;
	}
	DEBUG("( ] ) parsed");
	DEBUG("exited _array");
	return jnode;
}

static JsonNode* _boolean(char* buffer, ptrdiff_t length, ptrdiff_t offset) {
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
	json_allocator.free(allocator.context, boolString, strlen(boolString));
	DEBUG("exited _boolean");
	return jnode;
}

static JsonNode* _string(char* buffer, ptrdiff_t length, ptrdiff_t offset) {
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

static JsonNode* _number(char* buffer, ptrdiff_t length, ptrdiff_t offset) {
	DEBUG("entered _number");
	char* numberString = _scanWhile(jstream, _intPredicate);
	if (_fpeek(jstream) == '.') {
		char* appendee = _scanWhile(jstream, _realPredicate);
		numberString = strcat(numberString, appendee); // TODO: replace strcat
		char* end;
		double real = strtod(numberString, &end);
		json_allocator.free(allocator.context, appendee, strlen(appendee));
		json_allocator.free(allocator.context, numberString, strlen(numberString));
		#ifdef JSON4C_DEBUG
		printf("[%s:%d] ( %f ) parsed\n", __FILE__, __LINE__, real);
		#endif
		DEBUG("exited _number");
		return jnode_create(NULL, (JsonValue){JSON_REAL, .real = real});
	}
	int integer = atoi(numberString);
	json_allocator.free(allocator.context, numberString, strlen(numberString));
	#ifdef JSON4C_DEBUG
	printf("[%s:%d] ( %d ) parsed\n", __FILE__, __LINE__, integer);
	#endif
	DEBUG("exited _number");
	return jnode_create(NULL, (JsonValue){JSON_INT, .integer = integer});
}

static JsonNode* _null(char* buffer, ptrdiff_t length, ptrdiff_t offset) {
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
	json_allocator.free(allocator.context, nullString, strlen(nullString));
	DEBUG("exited _null");
	return jnode;
}
