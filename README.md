JSON4C
======

**WARNING**: This project is undergoing massive changes, expect incorrect or missing behaviour, and breaking changes regularly until a stable build is released.

**NOTE**: The serialization api is being reworked, expect regular breaking changes.

**NOTE**: The parser isn't yet compliant to the JSON standard in the way it parses numbers and strings. This is being worked on, but for most use cases it works as intended.

A simple, flexible JSON library written in pure C.

## Building

JSON4C is a unity build for ease of use. The only files you need to worry about are `json.h` and `json.c`, the rest exist only for orginization purposes.

1. Copy `src` to your project.
2. Rename `src` to something like `json4c`.
3. `#include "json4c\json.h"` when you want to use the library.
4. Add `json4c\json.c` to your compilation process.

## Examples

### Parsing

There are two functions provided for parsing JSON, `json_parse` and `json_parseFile`, below are their signatures.

~~~c
JsonNode* json_parse(char* buffer, ptrdiff_t length);
JsonNode* json_parseFile(char* path);
~~~

To extract data from a `JsonNode*` the library provides three functions, and some helper macros for type checking and casting.

~~~c
JsonNode* json_property(JsonNode* node, char* propertyName);
JsonNode* json_index(JsonNode* node, int index);
/*
	json_get is a combination of the other two functions,
	the call json_get(node, "friends", 0) roughly translates
	to json_index(json_property(node, "friends"), 0). In other
	words, json_get allows you to traverse the JSON tree in one go.
*/
JsonNode* json_get(JsonNode* node, ...);

/*
	The helper macros are:
	IS_INT(node)	AS_INT(node)
	IS_REAL(node)	AS_REAL(node)
	IS_STRING(node)	AS_STRING(node)
	IS_BOOL(node) 	AS_BOOL(node)
	IS_NULL(node) 	AS_NULL(node)
	IS_ARRAY(node) 	AS_ARRAY(node)
	IS_OBJECT(node) AS_OBJECT(node)
	IS_ERROR(node) 	
*/
~~~

#### Usage

A program that updates JSON files:

~~~c
#include <stdio.h>
#include <string.h>
#include "json4c/json.h"

// This example parses two JSON nodes, and updates 'background' with the changes in 'changes'.
int main(int argc, char* argv[]) {
	if (argc < 3) return 1;
	JsonNode* file = json_parseFile(argv[1]); // { "color": "grey", "opacity": 0.95 }
	JsonNode* changes = json_parse(argv[2], strlen(argv[2])); // { "color": "black" }
	if (IS_ERROR(file) || IS_ERROR(changes)) {
		json_node_free(file);
		json_node_free(changes);
		return 1;
	}
	// NOTE: a foreach function is in works
	for (int i = 0; i < changes->value.jcomplex.count; i++) {
		json_property(file, changes->value.jcomplex.nodes[i]->identifier)->value = changes->value.jcomplex.nodes[i]->value;
	}
	json_writeFile(file, argv[1], "w"); // Update the file.
	json_node_free(file);
	json_node_free(changes);
	return 0;
}
~~~

### Serialization

~~~c
#include <stdio.h>
#include "json4c/json.h"

int main(void) {
	/*
		The file created:
		{
			"test_status": "passing",
			"tests_passed": 123,
			"tests_failed": 0,
			"test_name": null,
			"files_tested": [
				"foo.xyz",
				"bar.xyz",
				"bazz.xyz"
			],
			"test_history": []
		}
	*/
	JsonNode* rootObject = json_object(
		"test_status", json_string("passing"),
		"tests_passed", json_int(123),
		"tests_failed", json_int(0),
		"test_name", json_null(),
		"files_tested", json_array(
			json_string("foo.xyz"),
			json_string("bar.xyz"),
			json_string("bazz.xyz")
		),
		"test_history", json_emptyArray()
	);
	// the last argument is for the write option, it is passed into fopen()
	json_writeFile("data/test.json", rootObject, "w");
	json_node_free(rootObject);
	return 0;
}
~~~

## Customization

### Macros

Below are all some of the macros the library uses that can be overidden by the user to suite their own needs.

~~~c
#define JSON_DEBUG // If you don't want to use a compilation flag
#define JSON_COMPLEX_DEFAULT_CAPACITY 16 // The default capacity of the dynamic array.
#define JSON_COMPLEX_GROW_MULTIPLIER 2 // How much the dynamic array grows by
~~~

Just use `-D` when compiling, e. `-D JSON_DEBUG -D JSON_COMPLEX_GROW_MULTIPLIER=4`. NOTE: More customization macros are in the works!

### Custom Allocators

If you want the library to use a custom allocator, you can use the `json_setAllocator` function.

~~~c
#include "json.h"
#include "your_allocator.h"

/*
	NOTE: 
	To support all kinds of custom allocators, the signatures for the
	allocator functions differ from you might expect. They are as follows:
	
	void* (*custom_alloc)(size_t size, void* instanceptr);
	void (*custom_free)(void* ptr, size_t size, void* instanceptr);
	void* (*custom_realloc)(void* ptr, size_t newSize, size_t oldSize, void* instanceptr);
*/

int main(void) {
	// NOTE: All arguments to json_setAllocator can be NULL except custom_alloc,
	// it is obviously recommended to supply a free function though.
	json_setAllocator(custom_alloc, custom_free, custom_realloc, custom_instance);
	JsonNode* jnode = json_parseFile("data\\foo.json"); // Allocated with custom_alloc
	json_node_free(jnode); // Freed with custom_free
	return 0;
}
~~~

Just make sure to set the allocator before any JSON allocations are made, and don't change it before all are freed.















