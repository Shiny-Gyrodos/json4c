#include <stdio.h>

#include "json.h"

int main() {
	JsonNode* node = json_parseFile("testdata\\array.json");
	JsonNode* searchedNode = json_get(node, 1, 5);
	printf("searchedNode = %s\n", searchedNode == NULL ? "NULL" : "non-null");
	if (searchedNode) {
		printf(
			"searchedNode->type = %d, searchedNode->identifier = %s\n", 
			searchedNode->type, 
			searchedNode->identifier
		);
	}
	printf("result = %f", searchedNode->value.real);
	return 0;
}
