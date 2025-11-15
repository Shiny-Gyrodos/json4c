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
	*capacity *= JSON_DYNAMIC_ARRAY_GROW_BY;
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
	if (*bytes != '\\') return '\0';
	bytes++;
	switch (*bytes) {
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
	}
	json_error_report("JSON_ERROR: json_utils_unescapeChar returned '\\0', invalid input");
	return '\0';
}

char* json_utils_escapeChar(char character) {
	char* string = json_allocator.alloc(3, json_allocator.context);
	if (!string) {
		json_error_reportCritical("JSON_ERROR: json_utils_escapeChar failed, alloc returned NULL");
		return NULL;
	}
	switch (character) {
		case '\t':
			return strcpy(string, "\\t");
		case '\r':
			return strcpy(string, "\\r");
		case '\n':
			return strcpy(string, "\\n");
		case '\b':
			return strcpy(string, "\\b");
		case '\f':
			return strcpy(string, "\\f");
		case '\\':
			return strcpy(string, "\\\\");
		case '/':
			return strcpy(string, "\\/");
		case '"':
			return strcpy(string, "\\\"");
	}
	json_error_report("JSON_ERROR: json_utils_escapeChar returned NULL, invalid input");
	json_allocator.free(string, 3, json_allocator.context);
	return NULL;
}

static bool _isEscapable(char c) {
	return c == '\t' || c == '\r' || c == '\n' || c == '\b' || c == '\f' || c == '/' || c == '\\';
}

char* json_utils_toEscaped(char* string) {
	ptrdiff_t addedMemory = 0;
	ptrdiff_t i;
	for (i = 0; string[i] != '\0'; i++) {
		if (_isEscapable(string[i]))
			addedMemory++;
	}
	char* newString = json_allocator.alloc(i + addedMemory + 1, json_allocator.context);
	if (!newString)
		json_error_reportCritical("JSON_ERROR: json_utils_toEscaped failed, alloc returned NULL");
	ptrdiff_t offset = 0;
	for (i = 0; string[i] != '\0'; i++, offset++) {
		if (!_isEscapable(string[i])) {
			newString[offset] = string[i];
			continue;
		}
		newString[offset++] = '\\';
		switch (string[i]) {
			case '\t':
				newString[offset] = 't';
				break;
			case '\r':
				newString[offset] = 'r';
				break;
			case '\n':
				newString[offset] = 'n';
				break;
			case '\b':
				newString[offset] = 'b';
				break;
			case '\f':
				newString[offset] = 'f';
				break;
			case '\\':
				newString[offset] = '\\';
				break;
			case '/':
				newString[offset] = '/';
				break;
			case '"':
				newString[offset] = '"';
				break;
		}
	}
	newString[offset] = '\0';
	return newString;
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
