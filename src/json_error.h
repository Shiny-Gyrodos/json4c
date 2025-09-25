#ifndef JSON4C_ERROR
#define JSON4C_ERROR

#include <stdio.h>

#include "json_types.h"

extern void (*json_error_onErrorReported)(char* errorMsg);
extern void (*json_error_onCriticalErrorReported)(char* errorMsg);
extern void (*json_error_onMaxErrors)(void);

void json_error_report(char*);
void json_error_reportCritical(char*);
void json_error_extract(JsonNode*);
void json_error_extractAndFree(JsonNode*);
void json_error_reset(void);
ptrdiff_t json_error_count(void);
char* json_error_pop(void); // pop the most recent error off the error stack
char** json_error_all(ptrdiff_t*);
void json_error_printAll(FILE*);

#endif // JSON4C_ERROR
