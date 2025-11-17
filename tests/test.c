#include <stdio.h>

#include "json_tests.h"

int main(void) {
	puts("tests started");
	json_initTests();
	json_runTests();
	puts("tests finished");
	return 0;
}
