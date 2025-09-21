JSON4C
======

**WARNING**: This library's parser is not yet compliant to the JSON standard in the way it parses numbers (it is too lenient) and strings (escape codes aren't handled correctly).

**NOTE**: The serialization API is still in progress, expect changes to the function signatures.

A simple, flexible JSON library written in pure C99.

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

Here is an example of the different ways you can parse JSON and fetch data from the nodes.

~~~c
#include <stdio.h>
#include <string.h>
#include "json4c/json.h"

int main(int argc, char* argv[]) {
	if (argc < 2) return 1;
	
	JsonNode* person = json_parse(argv[1], strlen(argv[1])); // { "name": "Lucas", "age": 34 }
	JsonNode* age = json_property(person, "age");
	printf("age = %d\n", AS_INT(age)); // Outputs "age = 34"
    json_node_free(person);
	
	JsonNode* primes = json_parseFile("testdata/primes.json"); // [ 1, 3, 5, 7, 11, 13, 17, 23 ]
	JsonNode* five = json_index(primes, 2);
	printf("the third prime number is %d\n", AS_INT(five));
    json_node_free(primes);
	
	/*
		testdata/house.json:
		{
			"number": 305,
			"price": 432000,
			"rooms": 11,
			"baths": 2.5,
			"bedrooms": 3,
			"owners": [
				"Joshua Davison",
				"Wendy Whitley",
				"Carrol Gretchen"
			]
		}
	*/
	JsonNode* house = json_parseFile("testdata/house.json");
	JsonNode* carrolGretchen = json_get(house, "owners", 2);
	printf("the third owner of the house was %s", AS_STRING(carrolGretchen));
    json_node_free(house);

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
#define JSON_DEBUG
#define JSON_DYNAMIC_ARRAY_CAPACITY 16
#define JSON_DYNAMIC_ARRAY_GROW_BY 2
#define JSON_BUFFER_CAPACITY 256
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



















