#include <string.h>

#include "json_types.h"
#include "json_config.h"

inline bool json_type_isComplex(JsonType type) {
	return type == JSON_OBJECT || type == JSON_ARRAY;
}


JsonNode* json_node_create(char* identifier, JsonValue value) {
	JsonNode* jnode = json_allocator.alloc(sizeof(JsonNode), json_allocator.context);
	if (!jnode) return NULL;
	jnode->identifier = identifier;
	jnode->value = value;
	if (json_type_isComplex(value.type)) {
		jnode->value.jcomplex.nodes = json_allocator.alloc(
			sizeof(JsonNode*) * JSON_COMPLEX_CAPACITY, 
			json_allocator.context
		);
		memset(jnode->value.jcomplex.nodes, 0, sizeof(JsonNode*) * JSON_COMPLEX_CAPACITY);
		jnode->value.jcomplex.max = JSON_COMPLEX_CAPACITY;
		jnode->value.jcomplex.count = 0;
	}
	return jnode;
}

void json_node_append(JsonNode* parent, JsonNode* child) {
	if (!parent || !child || !json_type_isComplex(parent->value.type)) return;
	if (parent->value.jcomplex.count >= parent->value.jcomplex.max) {
		void* temp = json_allocator.realloc(
			parent->value.jcomplex.nodes,
			parent->value.jcomplex.max * JSON_COMPLEX_GROW_MULTIPLIER,
			parent->value.jcomplex.max,
			json_allocator.context
		);
		if (!temp) return;
		parent->value.jcomplex.nodes = temp;
	}
	parent->value.jcomplex.nodes[parent->value.jcomplex.count] = child;
	parent->value.jcomplex.count++;
}

void json_node_free(JsonNode* jnode) {
	if (!jnode) return;
	if (json_type_isComplex(jnode->value.type)) {
		size_t i;
		for (i = 0; i < jnode->value.jcomplex.count; i++) {
			json_node_free(jnode->value.jcomplex.nodes[i]);
		}
	} else if (jnode->value.type == JSON_STRING) {
		json_allocator.free(jnode->value.string, strlen(jnode->value.string), json_allocator.context);
	}
	if (jnode->identifier) {
		json_allocator.free(jnode->identifier, strlen(jnode->identifier), json_allocator.context);		
	}
	json_allocator.free(jnode, sizeof(JsonNode), json_allocator.context);
}
