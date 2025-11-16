#include <string.h>

#include "json_types.h"
#include "json_error.h"
#include "json_utils.h"
#include "json_config.h"


static bool _safeStringEqual(const char*, const char*);


inline bool json_type_isComplex(JsonType type) {
	return type == JSON_OBJECT || type == JSON_ARRAY;
}


JsonNode* json_node_create(char* identifier, JsonValue value) {
	JsonNode* jnode = json_allocator.alloc(sizeof(JsonNode), json_allocator.context);
	if (!jnode) {
		json_error_reportCritical("JSON_ERROR: json_node_create failed, alloc returned NULL");
		return NULL;
	}
	jnode->identifier = identifier;
	jnode->value = value;
	if (json_type_isComplex(value.type)) {
		jnode->value.jcomplex.nodes = json_allocator.alloc(
			sizeof(JsonNode*) * JSON_DYNAMIC_ARRAY_CAPACITY, 
			json_allocator.context
		);
		if (!jnode->value.jcomplex.nodes) {
			json_error_reportCritical("JSON_ERROR: json_node_create failed, alloc returned NULL");
			json_node_free(jnode);
			return NULL;
		}
		memset(jnode->value.jcomplex.nodes, 0, sizeof(JsonNode*) * JSON_DYNAMIC_ARRAY_CAPACITY);
		jnode->value.jcomplex.max = JSON_DYNAMIC_ARRAY_CAPACITY;
		jnode->value.jcomplex.count = 0;
	}
	return jnode;
}

void json_node_append(JsonNode* parent, JsonNode* child) {
	if (!parent || !child || !json_type_isComplex(parent->value.type)) return;
	if (parent->value.jcomplex.count >= parent->value.jcomplex.max) {
		void* temp = json_allocator.realloc(
			parent->value.jcomplex.nodes,
			parent->value.jcomplex.max * sizeof(JsonNode*) * JSON_DYNAMIC_ARRAY_GROW_BY,
			parent->value.jcomplex.max * sizeof(JsonNode*),
			json_allocator.context
		);
		if (!temp) {
			json_error_reportCritical("JSON_ERROR: json_node_append failed, realloc returned NULL");
			return;
		}
		parent->value.jcomplex.nodes = temp;
		parent->value.jcomplex.max *= JSON_DYNAMIC_ARRAY_GROW_BY;
	}
	parent->value.jcomplex.nodes[parent->value.jcomplex.count] = child;
	parent->value.jcomplex.count++;
}

// NOTE: AS_OBJECT and AS_ARRAY can be used interchangeably,
// since in both cases the data is represented in the same way.
ptrdiff_t json_node_childrenCount(const JsonNode* node) {
	if (!node || !json_type_isComplex(node->value.type)) return 0;
	ptrdiff_t count = AS_COMPLEX(node).count;
	for (ptrdiff_t i = 0; i < AS_COMPLEX(node).count; i++) {
		count += json_node_childrenCount(AS_COMPLEX(node).nodes[i]);
	}
	return count;
}

bool json_node_equals(const JsonNode* node1, const JsonNode* node2) {
	// Immediate checks
	if (node1 == node2) return true;
	if (node1->value.type != node2->value.type) return false;
	if (!_safeStringEqual(node1->identifier, node2->identifier)) return false;
	
	if (json_type_isComplex(node1->value.type)) {
		if (node1->value.jcomplex.count != node2->value.jcomplex.count) return false;
		for (ptrdiff_t i = 0; i < node1->value.jcomplex.count; i++) {
			if (!json_node_equals(node1->value.jcomplex.nodes[i], node2->value.jcomplex.nodes[i]))
				return false;
		}
		return true;
	}
	
	switch (node1->value.type) {
		case JSON_INT:
			return node1->value.integer == node2->value.integer;
		case JSON_REAL:
			return node1->value.real == node2->value.real;
		case JSON_BOOL:
			return node1->value.boolean == node2->value.boolean;
		case JSON_STRING:
			// not _safeStringEqual becuase value.string should never be NULL
			return strcmp(node1->value.string, node2->value.string) == 0;
		default:
			json_error_report("JSON_ERROR: unexpected node type in 'json_node_equals'");
			return false;
	}
}

void json_node_free(JsonNode* jnode) {
	if (!jnode) return;
	if (json_type_isComplex(jnode->value.type)) {
		ptrdiff_t i;
		for (i = 0; i < jnode->value.jcomplex.count; i++) {
			json_node_free(jnode->value.jcomplex.nodes[i]);
		}
		json_allocator.free(
			jnode->value.jcomplex.nodes, 
			jnode->value.jcomplex.count * sizeof(JsonNode*),
			json_allocator.context
		);
	} else if (jnode->value.type == JSON_STRING) {
		json_allocator.free(jnode->value.string, strlen(jnode->value.string), json_allocator.context);
	}
	if (jnode->identifier) {
		json_allocator.free(jnode->identifier, strlen(jnode->identifier), json_allocator.context);		
	}
	json_allocator.free(jnode, sizeof(JsonNode), json_allocator.context);
}


static bool _safeStringEqual(const char* s1, const char* s2) {
	if (!s1 && !s2) return true;
	if (!s1 || !s2) return false;
	return strcmp(s1, s2) == 0;
}
