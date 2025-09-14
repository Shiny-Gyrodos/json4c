#include "json_utils.h"

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

inline char json_buf_peek(char* buffer, ptrdiff_t length, ptrdiff_t offset) {
	return offset < length ? buffer[offset] : buffer[length - 1];
}
