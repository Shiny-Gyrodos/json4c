#include <stddef.h>

#include "json_config.h"
#include "json_serializer.h"
#include "json_error.h"

struct {
	char* errors[JSON_MAX_ERRORS_RECORDED];
	ptrdiff_t count;
} errorStack = { .count = 0 };


void json_error_report(char* errorMsg) {
	errorStack.errors[errorStack.count++] = errorMsg;
}

void json_error_extract(JsonNode* node) {
	char* errorMsg = json_toString(node);
	json_error_report(errorMsg);
}

void json_error_extractAndFree(JsonNode* node) {
	json_error_extract(node);
	json_node_free(node);
}

ptrdiff_t json_error_reset(void) {
	ptrdiff_t i;
	for (i = 0; i < errorStack.count; i++) {
		errorStack.errors[i] = NULL;
	}
	errorStack.count = 0;
}

ptrdiff_t json_error_count(void) {
	return errorStack.count;
}

char* json_error_pop(void) {
	return errorStack.errors[--errorStack.count];
}

const char** json_error_all(ptrdiff_t* count) {
	*count = errorStack.count;
	return errorStack.errors;
}

void json_error_printAll(FILE* stream) {
	ptrdiff_t i;
	for (i = 0; i < errorStack.count; i++) {
		fputs(errorStack.errors[i], stream);
	}
}
