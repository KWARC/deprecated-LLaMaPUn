#include "../stopwords.h"
#include <stdio.h>
#include <json-c/json.h>


int main(void) {
	read_stopwords(json_object_from_file("../stopwords.json"));
	int errors = 0;

	if (!is_stopword("the")) {
		fprintf(stderr, "test stopwords -- wrong result: 'the' should be a stopword\n");
		errors++;
	}
	if (!is_stopword("be")) {
		fprintf(stderr, "test stopwords -- wrong result: 'the' should be a stopword\n");
		errors++;
	}
	if (is_stopword("group")) {
		fprintf(stderr, "test stopwords -- wrong result: 'group' shouldn't be a stopword\n");
		errors++;
	}
	if (!is_stopword("in")) {
		fprintf(stderr, "test stopwords -- wrong result: 'in' should be a stopword\n");
		errors++;
	}

	free_stopwords();
	if (errors) return 1;
	return 0;
}