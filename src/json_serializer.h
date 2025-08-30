#ifndef JSON4C_SERIALIZER
#define JSON4C_SERIALIZER

enum JsonSerializationOptions {
	OPT_DEFAULT	 			= 	1 << 0,
	OPT_NO_WHITESPACE 		= 	1 << 1,
	OPT_SINGLE_LINE_EMPTIES = 	1 << 2
};

JsonNode* json_object_impl(JsonNode**); // shouldn't be called, use the macro wrapper instead
#define json_object(...) json_object_impl((JsonNode*[]){__VA_ARGS__, NULL})
#define json_emptyObject() jnode_create(NULL, (JsonValue){JSON_OBJECT, 0})
JsonNode* json_array_impl(JsonNode**); // shouldn't be called, use the macro wrapper instead
#define json_array(...) json_array_impl((JsonNode*[]){__VA_ARGS__, NULL})
#define json_emptyArray() jnode_create(NULL, (JsonValue){JSON_ARRAY, 0})
JsonNode* json_bool(bool);
JsonNode* json_int(int);
JsonNode* json_real(double);
JsonNode* json_null(void);
JsonNode* json_string(char*);
bool json_write(char* buffer, JsonNode* jnode, enum JsonSerializationOptions jopts);

#endif // JSON4C_SERIALIZER
