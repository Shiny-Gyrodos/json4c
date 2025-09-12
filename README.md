JSON4C
======

WARNING: This project is undergoing massive changes, expect incorrect / missing behaviour, and breaking changes regularly until a stable build is released.

A simple, flexible JSON library written in pure C.

## Building

JSON4C is a unity build for ease of use. The only files you need to worry about are `json.h` and `json.c`, the rest are exist only for code orginization.

1. Copy `src` to your project.
2. Rename `src` to something like `json4c`
3. `#include "json4c\json.h"` when you want to use the library.
4. Add `json4c\json.c` to your compilation process.

## Examples

### Deserialization

#### Using 'json_property'

~~~c
#include <stdio.h>

#include "json.h"

int main(void) {
	// Parse the file and print the "age" property
	JsonNode* person = json_parseFile("data\\person.json");
	JsonNode* age = json_property(person, "age");
	printf("age = %d\n", AS_INT(age)); // Would print the age property.
	json_free(person);
	return 0;
}
~~~

#### Using 'json_index'

~~~c
#include <stdio.h>

#include "json.h"

int main(void) {
	// Parse the file and print the number at the 3rd index.
	JsonNode* numberArray = json_parseFile("data\\numbers.json");
	JsonNode* number = json_index(numbers, 3);
	printf("number = %d\n", AS_INT(number)); // Would print the number at the 3rd index.
	json_free(numbers);
	return 0;
}
~~~

#### Using 'json_get' to traverse the tree.

~~~c
#include <stdio.h>

#include "json.h"

int main(void) {
	// Parse the file and traverse the tree.
	JsonNode* subscribers = json_parseFile("data\\subscribers.json");
	JsonNode* email = json_get(subscribers, 2, 6, "email"); // The first number denotes the amount of following arguments.
	printf("email = %s\n", AS_STRING(email)); // Would print the 6th person's email property.
	json_free(subscribers);
	return 0;
}
~~~

### Serialization

~~~c
#include <stdio.h>

#include "json.h"

int main(void) {
	/*
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
		json_string("test_status"), json_string("passing"),
		json_string("tests_passed"), json_int(123),
		json_string("tests_failed"), json_int(0),
		json_string("test_name"), json_null(),
		json_string("files_tested"), json_array(
			json_string("foo.xyz"),
			json_string("bar.xyz"),
			json_string("bazz.xyz")
		),
		json_string("test_history"), json_emptyArray()
	);
	// The "" indicates the amount of indentation you want to start with.
	json_write("data\\test.json", rootObject, "");
	json_free(rootObject);
	return 0;
}
~~~

## Compilation Flags

Use `-D JSON_DEBUG` to print parsing info to stdout.

## Customization

### Macros

When including json.h you may define some macros for customization, below are all the options listed with their default values.

~~~c
#define JSON_DEBUG // If you don't want to use a compilation flag
#define JSON_COMPLEX_DEFAULT_CAPACITY 16 // The default capacity of the dynamic array.
#define JSON_COMPLEX_GROW_MULTIPLIER 2 // How much the dynamic array grows by
#define JSON_ALLOCATOR_DEFAULT (struct Allocator){json_std_alloc, json_std_free, json_std_realloc, NULL}
#include "json.h"
~~~

### Custom Allocators

If you don't want to set the custom allocators with preprocessor macros, you can instead use the json_setAllocator function.

~~~c
#include "json.h"
#include "your_allocator.h"

int main(void) {
	json_setAllocator(your_allocator_alloc, your_allocator_free, your_allocator_realloc, your_allocator_instance);
	JsonNode* jnode = json_parseFile("data\\foo.json"); // Allocated with allocator_alloc
	json_free(jnode); // Freed with allocator_free
	return 0;
}
~~~

Just make sure to set the allocator before any JSON allocations are made, and don't change it before all are freed.



