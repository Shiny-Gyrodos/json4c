#include "json_serializer.h"

JsonNode* json_object_impl(JsonNode** jnodes) {
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

JsonNode* json_array_impl(JsonNode** jnodes) {
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

inline JsonNode* json_real(double real) {
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
				char* newIndent = allocator.json_alloc(allocator.context, strlen(indent) + 1);
				sprintf(newIndent, "%s\t", indent);
				_serialize(
					jsonFile, 
					jnode->value.jcomplex.nodes[i], 
					newIndent, 
					jnode->value.jcomplex.nodes[i + 1] == NULL ? "\n" : ",\n"
				);
				allocator.json_free(allocator.context, newIndent, strlen(newIndent));
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
