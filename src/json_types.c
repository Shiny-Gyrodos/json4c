#include <string.h>

#include "json_types.h"
#include "json_config.h"

inline bool json_type_isComplex(JsonType type) {
	return type == JSON_OBJECT || type == JSON_ARRAY;
}


JsonNode* jnode_create(char* identifier, JsonValue value) {
	JsonNode* jnode = allocator.json_alloc(sizeof(JsonNode), allocator.context);
	if (!jnode) return NULL;
	jnode->identifier = identifier;
	jnode->value = value;
	if (json_type_isComplex(value.type)) {
		jnode->value.jcomplex.nodes = allocator.json_alloc(sizeof(JsonNode*) * JSON_COMPLEX_DEFAULT_CAPACITY, allocator.context);
		memset(jnode->value.jcomplex.nodes, 0, sizeof(JsonNode*) * JSON_COMPLEX_DEFAULT_CAPACITY);
		jnode->value.jcomplex.max = JSON_COMPLEX_DEFAULT_CAPACITY;
		jnode->value.jcomplex.count = 0;
	}
	return jnode;
}

void jnode_append(JsonNode* parent, JsonNode* child) {
	if (!parent || !child || !json_type_isComplex(parent->value.type)) return;
	if (parent->value.jcomplex.count >= parent->value.jcomplex.max) {
		void* temp = allocator.json_realloc(
			parent->value.jcomplex.nodes,
			parent->value.jcomplex.max * JSON_COMPLEX_GROW_MULTIPLIER,
			parent->value.jcomplex.max,
			allocator.context
		);
		if (!temp) return;
		parent->value.jcomplex.nodes = temp;
	}
	parent->value.jcomplex.nodes[parent->value.jcomplex.count] = child;
	parent->value.jcomplex.count++;
}

void jnode_free(JsonNode* jnode) {
	if (!jnode) return;
	if (json_type_isComplex(jnode->value.type)) {
		int i;
		for (i = 0; i < jnode->value.jcomplex.count; i++) {
			jnode_free(jnode->value.jcomplex.nodes[i]);
		}
	} else if (jnode->value.type == JSON_STRING) {
		allocator.json_free(jnode->value.string, strlen(jnode->value.string), allocator.context);
	}
	allocator.json_free(jnode->identifier, strlen(jnode->identifier), allocator.context);
	allocator.json_free(jnode, sizeof(JsonNode), allocator.context);
}
