#include <string.h>

#include "json_utils.h"

char* json_utils_scanUntil(char* buffer, char* delimiters, Allocator allocator) {
	if (!stream || !delimiters) return NULL;
	char* string = allocator.json_alloc(allocator.context, 8);
	if (!string) return NULL;
	size_t max = 8;
	size_t count = 0;
	int nextChar;
	while ((nextChar = fgetc(stream)) != EOF) {
		if (count >= max) {
			max *= 2;
			char* temp = realloc(string, max);
			if (!temp) {
				allocator.json_free(allocator.context, string, strlen(string));
				return NULL;
			}
			string = temp;
		}
		if (strchr(delimiters, nextChar)) {
			ungetc(nextChar, stream);
			string[count] = '\0';
			return string;
		}
		string[count++] = nextChar;
	}
	allocator.json_free(allocator.context, string, strlen(string));
	return NULL;
}

char* json_utils_scanWhile(char* buffer, bool (predicate)(char), Allocator allocator) {
	if (!stream || !predicate) return NULL;
	char* string = allocator.json_alloc(allocator.context, 8);
	if (!string) return NULL;
	size_t max = 8;
	size_t count = 0;
	int nextChar;
	while ((nextChar = fgetc(stream)) != EOF) {
		if (count >= max) {
			max *= 2;
			char* temp = realloc(string, max);
			if (!temp) {
				allocator.json_free(allocator.context, string, strlen(string));
				return NULL;
			}
			string = temp;
		}
		if (!predicate(nextChar)) {
			ungetc(nextChar, stream);
			string[count] = '\0';
			return string;
		}
		string[count++] = nextChar;
	}
	allocator.json_free(allocator.context, string, strlen(string));
	return NULL;
}
