#include "json_utils.h"
#include "json_allocator.h"

void json_utils_dynAppendStr_impl(char** buffer, ptrdiff_t* length, ptrdiff_t* offset, char** strings) {
	int i = 0;
	char* currentString = strings[i];
	while (currentString != NULL) {
		int j;
		for (j = 0; currentString[j] != '\0'; j++) {
			if (*offset >= *length) {
				void* temp = json_allocator.realloc(
					*buffer, 
					*length * 2, 
					*length, 
					json_allocator.context
				);
				if (!temp) {
					json_allocator.free(*buffer, *length, json_allocator.context);
					*buffer = NULL;
					return;
				}
				*buffer = temp;
				*length *= 2;
			}
			(*buffer)[(*offset)++] = currentString[j]; 
		}
		i++;
		currentString = strings[i];
	}
}


inline bool json_buf_expect(char c, char* buffer, ptrdiff_t length, ptrdiff_t* offset) {
	return c == json_buf_get(buffer, length, offset);
}

inline char json_buf_get(char* buffer, ptrdiff_t length, ptrdiff_t* offset) {
	return *offset < length ? buffer[(*offset)++] : buffer[length - 1];
}

// TODO: It isn't clear enough from the return value when this function fails.
inline char json_buf_put(char c, char* buffer, ptrdiff_t length, ptrdiff_t* offset) {
	return *offset - 1 >= 0 && *offset - 1 < length ? buffer[--(*offset)] = c : '\0';
}

// TODO: what if '\0' is just part of the string?
inline bool json_buf_putstr(char* string, char* buffer, ptrdiff_t length, ptrdiff_t* offset) {
	if (*offset + (ptrdiff_t)strlen(string) >= length) return false;
	for (int i = 0; string[i] != '\0' && json_buf_put(string[i], buffer, length, offset); i++);
	return true;
}

inline char json_buf_peek(char* buffer, ptrdiff_t length, ptrdiff_t offset) {
	return offset < length ? buffer[offset] : buffer[length - 1];
}
