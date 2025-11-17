#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "../src/json.h"
#include "json_tests.h"

#define EXPECT(x, y)															\
	do {																		\
		if (x != y) {															\
			fprintf(															\
				stderr, 														\
				"Assertion failed: " #x " == " #y ", in %s at [%s:%d]\n", 		\
				__func__, __FILE__, __LINE__									\
			);																	\
		}																		\
	} while(0)
		
#define TO_BE(x) x

#define DATA_PATH "./data/"
#define GENERATED_DATA_PATH DATA_PATH "generated/"


void json_initTests(void) {
	// nothing here yet...
}

void json_runTests(void) {
	json_runNodeTests();
	json_runParserTests(); 
	json_runSerializerTests();
	json_runUtilsTests();
}

// Tests to ensure node construction behaves as intended.
void json_runNodeTests(void) {
	JsonNode* array = json_node_create(NULL, (JsonValue){JSON_ARRAY, {0}});
	EXPECT(IS_ARRAY(array),				TO_BE(true));
	EXPECT(array->identifier, 			TO_BE(NULL));
	EXPECT(AS_ARRAY(array).max,			TO_BE(JSON_DYNAMIC_ARRAY_CAPACITY));
	EXPECT(AS_ARRAY(array).count, 		TO_BE(0));
	EXPECT(array->value.type, 			TO_BE(JSON_ARRAY));
	
	JsonNode* node = json_node_create(NULL, (JsonValue){JSON_NULL, {0}});
	json_node_append(array, node);
	node = AS_ARRAY(array).nodes[0];
	EXPECT(AS_ARRAY(array).count,		TO_BE(1));
	EXPECT(IS_NULL(node),				TO_BE(true));
	
	for (ptrdiff_t i = 0; i < JSON_DYNAMIC_ARRAY_CAPACITY + 1; i++) {
		node = json_node_create(NULL, (JsonValue){JSON_INT, {i}});
		json_node_append(array, node);
	}
	ptrdiff_t count = json_node_childrenCount(array);
	EXPECT(AS_ARRAY(array).max, 		TO_BE(JSON_DYNAMIC_ARRAY_CAPACITY * JSON_DYNAMIC_ARRAY_GROW_BY));
	EXPECT(AS_ARRAY(array).count,		TO_BE(count));
	
	char* ip = "121.1265.75123";
	node = json_object("ip", json_string(ip));
	json_node_append(array, node);
	count = json_node_childrenCount(array);
	EXPECT(IS_OBJECT(node),				TO_BE(true));
	EXPECT(node->value.type,			TO_BE(JSON_OBJECT));
	EXPECT(AS_OBJECT(node).count,		TO_BE(1));
	EXPECT(AS_ARRAY(array).count,		TO_BE(count - 1)); // count - 2 becuase we added { "ip": "..." }
	
	node = json_get(array, AS_ARRAY(array).count - 1, "ip");
	EXPECT(IS_STRING(node),				TO_BE(true));
	EXPECT(strcmp(AS_STRING(node), ip), TO_BE(0));

	JsonNode* obj1 = json_object("name", json_string("clancy"));
	JsonNode* obj2 = json_object("name", json_string("clancy"));
	bool isValueEqual = json_node_equals(obj1, obj2);
	bool isRefEqual = json_node_equals(obj1, obj1);
	bool isUnequal = !json_node_equals(obj1, array); // NOTE: notice the '!'
	EXPECT(isValueEqual,				TO_BE(true));
	EXPECT(isRefEqual,					TO_BE(true));
	EXPECT(isUnequal,					TO_BE(true));
	
	json_node_free(array);
	json_node_free(obj1);
	json_node_free(obj2);
} 

// Tests to ensure parsing behaves as intended.
// NOTE: json_parse isn't tested because json_parseFile calls it.
void json_runParserTests(void) {
	JsonNode* expectedNumbers = json_array(
		json_int(1),
		json_int(2),
		json_int(3),
		json_int(4),
		json_int(5)
	);
	JsonNode* numbers = json_parseFile(DATA_PATH "numbers.json");
	JsonNode* first = json_index(numbers, 0);
	JsonNode* last = json_index(numbers, 4);
	JsonNode* pastLast = json_index(numbers, 8);
	bool equals = json_node_equals(expectedNumbers, numbers);
	EXPECT(IS_ERROR(numbers),			TO_BE(false));
	EXPECT(IS_ARRAY(numbers),			TO_BE(true));
	EXPECT(AS_INT(first),				TO_BE(1));
	EXPECT(AS_INT(last),				TO_BE(5));
	EXPECT(pastLast,					TO_BE(NULL));
	EXPECT(equals,						TO_BE(true));
	json_node_free(expectedNumbers);
	json_node_free(numbers);
	
	JsonNode* expectedServiceConfig = json_object(
		"super_secret_key", 	json_string("..."),
		"telemetry",			json_bool(false),
		"analytics",			json_bool(true),
		"ai_nonsense_amount",	json_int(100),
		"supported_platforms",	json_array(
			json_string("Mac"),
			json_string("Windows"),
			json_string("Unix-Based"),
			json_string("Android"),
			json_string("Web")
		)
	);
	JsonNode* serviceConfig = json_parseFile(DATA_PATH "service_config.json");
	equals = json_node_equals(expectedServiceConfig, serviceConfig);
	ptrdiff_t childCount = json_node_childrenCount(serviceConfig);
	EXPECT(IS_ERROR(serviceConfig),		TO_BE(false));
	EXPECT(IS_OBJECT(serviceConfig),	TO_BE(true));
	EXPECT(childCount,					TO_BE(10));
	EXPECT(equals,						TO_BE(true));
	json_node_free(expectedServiceConfig);
	json_node_free(serviceConfig);
	
	JsonNode* error = json_parseFile(DATA_PATH "invalid.json");
	childCount = json_node_childrenCount(error);
	EXPECT(IS_ERROR(error),				TO_BE(true));
	EXPECT(childCount,					TO_BE(0));
	json_node_free(error);
}

// Tests to ensure serialization behaves as intended.
// TODO: add more test cases to this function
void json_runSerializerTests(void) {
	JsonNode* array = json_array(
		json_int(1),
		json_int(2),
		json_int(3),
		json_int(4),
		json_int(5)
	);
	json_writeFile(array, GENERATED_DATA_PATH "numbers.json", JSON_WRITE_PRETTY);
	JsonNode* parsedArray = json_parseFile(GENERATED_DATA_PATH "numbers.json");
	bool nodesEqual = json_node_equals(array, parsedArray);
	EXPECT(IS_ERROR(parsedArray),		TO_BE(false));
	EXPECT(nodesEqual,					TO_BE(true));
	json_node_free(array);
	json_node_free(parsedArray);
	
	JsonNode* nestedObject = json_object(
		"child", json_object(
			"child", json_null()
		)
	);
	json_writeFile(nestedObject, GENERATED_DATA_PATH "nested_object.json", JSON_WRITE_CONDENSED);
	JsonNode* parsedNestedObject = json_parseFile(GENERATED_DATA_PATH "nested_object.json");
	nodesEqual = json_node_equals(nestedObject, parsedNestedObject);
	EXPECT(IS_ERROR(parsedNestedObject),TO_BE(false));
	EXPECT(nodesEqual,					TO_BE(true));
	json_node_free(nestedObject);
	json_node_free(parsedNestedObject);
}

// Tests to ensure utils functions behave as intended.
void json_runUtilsTests(void) {
	// ensureCapacity
	{
		char* buffer = malloc(1);
		ptrdiff_t length = 1;
		ptrdiff_t offset = 1;
		json_utils_ensureCapacity(&buffer, &length, offset);
		EXPECT(length,						TO_BE(8));
		while (offset < 8) {
			buffer[offset] = (char)offset++;
		}
		json_utils_ensureCapacity(&buffer, &length, offset);
		EXPECT(length,						TO_BE(8 * JSON_DYNAMIC_ARRAY_GROW_BY));
		free(buffer);
	}
	
	// dynAppendStr
	{
		const char* greeting = "Hello, World!";
		ptrdiff_t length = strlen(greeting);
		ptrdiff_t offset = 0;
		char* string = malloc(length + 1);
		json_utils_dynAppendStr(&string, &length, &offset, "Hello", ",", " ", "World", "!");
		string[length] = '\0';
		EXPECT(strcmp(string, greeting),	TO_BE(0));
		free(string);
	}
	
	// unescapeChar
	{
		char tab = json_utils_unescapeChar("\\t");
		char backslash = json_utils_unescapeChar("\\\\");
		char invalid1 = json_utils_unescapeChar("not_escapable");
		char invalid2 = json_utils_unescapeChar("\\almost_escapable");
		EXPECT(tab,							TO_BE('\t'));
		EXPECT(backslash,					TO_BE('\\'));
		EXPECT(invalid1,					TO_BE('\0'));
		EXPECT(invalid2,					TO_BE('\0'));
	}
	
	// escapeChar
	{
		char* escTab = json_utils_escapeChar('\t');
		char* escBackslash = json_utils_escapeChar('\\');
		char* invalid1 = json_utils_escapeChar('Y');
		char* invalid2 = json_utils_escapeChar('#');
		EXPECT(strcmp(escTab, "\\t"),		TO_BE(0));
		EXPECT(strcmp(escBackslash, "\\\\"),TO_BE(0));
		EXPECT(invalid1,					TO_BE(NULL));
		EXPECT(invalid2,					TO_BE(NULL));
		free(escTab);
		free(escBackslash);
	}
	
	// toEscaped
	{
		const char* expected = "\\\\\\t\\r";
		char* escaped = json_utils_toEscaped("\\\t\r");
		EXPECT(strcmp(expected, escaped),	TO_BE(0));
		free(escaped);
	}
	
	// buf_x
	{
		char buffer[] = "test1 test2 test3";
		ptrdiff_t length = strlen(buffer);
		ptrdiff_t offset = 0;
		bool isExpected = json_buf_expect('t', buffer, length, &offset);
		offset = length + 1;
		char last = json_buf_get(buffer, length, &offset);
		char terminatingChar = json_buf_unget('%', buffer, length, &offset);
		offset = 1;
		char dollar1 = json_buf_unget('$', buffer, length, &offset);
		char dollar2 = json_buf_peek(buffer, length, offset);
		EXPECT(isExpected,					TO_BE(true));
		EXPECT(last,						TO_BE('3'));
		EXPECT(terminatingChar,				TO_BE('\0'));
		EXPECT(dollar1,						TO_BE('$'));
		EXPECT(dollar2,						TO_BE('$'));
	}
}
