#include "json_utils.h"
#include "json_allocator.h"
#include "json_error.h"
#include "json_config.h"

void json_utils_ensureCapacity_impl(void** ptr, size_t size, ptrdiff_t* capacity, ptrdiff_t count) {
	if (count < *capacity || !ptr || !(*ptr)) return;
	void* temp = json_allocator.realloc(
		*ptr,
		*capacity * size * JSON_DYNAMIC_ARRAY_GROW_BY,
		*capacity * size,
		json_allocator.context
	);
	if (!temp) {
		json_error_report("JSON_ERROR: json_utils_ensureCapacity failed, realloc returned NULL");
		return;
	}
	*ptr = temp;
}

void json_utils_dynAppendStr_impl(char** buffer, ptrdiff_t* length, ptrdiff_t* offset, char** strings) {
	ptrdiff_t i = 0;
	char* currentString = strings[i];
	while (currentString != NULL) {
		ptrdiff_t j;
		for (j = 0; currentString[j] != '\0'; j++) {
			json_utils_ensureCapacity(buffer, length, *offset);
			(*buffer)[(*offset)++] = currentString[j]; 
		}
		i++;
		currentString = strings[i];
	}
}


char json_utils_unescapeChar(char* bytes) {
	if (*bytes != '\\') return NULL;
	bytes++;
	switch (*chars) {
		case 't':
			return '\t';
		case 'r':
			return '\r';
		case 'n':
			return '\n';
		case 'b':
			return '\b';
		case 'f':
			return '\f';
		case '"':
			return '"';
		case '/':
			return '/';
		case '\\':
			return '\\';
		case 'u':
			json_error_report("JSON_ERROR: json_utils_unescapeChar failed, escaped hex digits not yet supported");
			return '\0'
	}
}

char* json_utils_escapeChar(char* character) {
	
}


inline bool json_buf_expect(char c, char* buffer, ptrdiff_t length, ptrdiff_t* offset) {
	return c == json_buf_get(buffer, length, offset);
}

inline char json_buf_get(char* buffer, ptrdiff_t length, ptrdiff_t* offset) {
	return *offset < length ? buffer[(*offset)++] : buffer[length - 1];
}

// TODO: It isn't clear enough from the return value when this function fails.
inline char json_buf_unget(char c, char* buffer, ptrdiff_t length, ptrdiff_t* offset) {
	return *offset - 1 >= 0 && *offset - 1 < length ? buffer[--(*offset)] = c : '\0';
}

inline char json_buf_peek(char* buffer, ptrdiff_t length, ptrdiff_t offset) {
	return offset < length ? buffer[offset] : buffer[length - 1];
}
