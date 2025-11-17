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
		if (AS_COMPLEX(node1).count != AS_COMPLEX(node2).count) return false;
		for (ptrdiff_t i = 0; i < AS_COMPLEX(node1).count; i++) {
			if (!json_node_equals(AS_COMPLEX(node1).nodes[i], AS_COMPLEX(node2).nodes[i]))
				return false;
		}
		return true;
	}
	
	switch (node1->value.type) {
		case JSON_INT:
			return AS_INT(node1) == AS_INT(node2);
		case JSON_REAL:
			return AS_REAL(node1) == AS_REAL(node2);
		case JSON_BOOL:
			return AS_BOOL(node1) == AS_BOOL(node2);
		case JSON_STRING:
			// not _safeStringEqual becuase value.string should never be NULL
			return strcmp(AS_STRING(node1), AS_STRING(node2)) == 0;
		case JSON_NULL:
			return true;
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


JsonNode* json_object_impl(void** ptrs) {
	JsonNode* jobject = json_node_create(NULL, (JsonValue){JSON_OBJECT, {0}});
	ptrdiff_t i;
	char* identifier = NULL;
	for (i = 0; ptrs[i]; i++) {
		if (i % 2 == 0) { // starting with the first, every other value is an identifier (char*)
			identifier = (char*)ptrs[i];
		} else {
			JsonNode* jnode = (JsonNode*)ptrs[i];
			int length = strlen(identifier);
			void* temp = json_allocator.alloc(length + 1, json_allocator.context);
			if (!temp) {
				json_node_free(jobject);
				json_error_reportCritical("JSON_ERROR: json_object failed, alloc returned NULL");
				return NULL;
			}
			jnode->identifier = temp;
			strcpy(jnode->identifier, identifier);
			identifier = NULL;
			json_node_append(jobject, jnode);
		}
	}
	return jobject;
}

JsonNode* json_array_impl(JsonNode** jnodes) {
	JsonNode* jarray = json_node_create(NULL, (JsonValue){JSON_ARRAY, {0}});
	ptrdiff_t i;
	for (i = 0; jnodes[i]; i++) {
		json_node_append(jarray, jnodes[i]);
	}
	return jarray;
}

inline JsonNode* json_bool(bool boolean) {
	return json_node_create(NULL, (JsonValue){JSON_BOOL, .boolean = boolean});
}

inline JsonNode* json_int(int64_t integer) {
	return json_node_create(NULL, (JsonValue){JSON_INT, .integer = integer});
}

inline JsonNode* json_real(double real) {
	return json_node_create(NULL, (JsonValue){JSON_REAL, .real = real});
}

inline JsonNode* json_null(void) {
	return json_node_create(NULL, (JsonValue){JSON_NULL, {0}});
}

inline JsonNode* json_string(char* string) {
	return json_node_create(NULL, (JsonValue){JSON_STRING, .string = string});
}


JsonNode* json_property(JsonNode* jnode, char* identifier) {
	if (!jnode || !identifier || jnode->value.type != JSON_OBJECT) return NULL;
	ptrdiff_t i;
	for (i = 0; i < jnode->value.jcomplex.count; i++) {
		if (strcmp(jnode->value.jcomplex.nodes[i]->identifier, identifier) == 0) {
			return jnode->value.jcomplex.nodes[i];
		}
	}
	return NULL;
}

JsonNode* json_index(JsonNode* jnode, ptrdiff_t index) {
	if (!jnode || jnode->value.type != JSON_ARRAY || index >= jnode->value.jcomplex.count) 
		return NULL;
	return jnode->value.jcomplex.nodes[index];
}

#define TERMINATOR -1
JsonNode* json_get_impl(JsonNode* root, ...) {
	va_list args;
	va_start(args, root);
	while (true) {
		if (!root) {
			va_end(args);
			return NULL;
		}
		if (root->value.type == JSON_OBJECT) {
			char* identifier = va_arg(args, char*);
			if (identifier == (char*)TERMINATOR) 
				break;
			root = json_property(root, identifier);
		} else if (root->value.type == JSON_ARRAY) {
			intptr_t index = va_arg(args, intptr_t);
			if (index == TERMINATOR) 
				break;
			root = json_index(root, index);
		} else {
			if (va_arg(args, intptr_t) == TERMINATOR) 
				break;
			va_end(args);
			return NULL;
		}
	}
	va_end(args);
	return root;
}
#undef TERMINATOR


static bool _safeStringEqual(const char* s1, const char* s2) {
	if (!s1 && !s2) return true;
	if (!s1 || !s2) return false;
	return strcmp(s1, s2) == 0;
}
