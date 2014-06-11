#include "../normalizer.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


int main(void) {
	init_normalizer();

	char *normalized;

	normalize("there are numbers", &normalized);

	if (strcmp(normalized, "there be number")) {
		fprintf(stderr, "test normalizer -- wrong result: %s\n", normalized);
		free(normalized);
		return 1;
	}

	normalize("Let x be the greatest common denominator of y we are going to prove the following", &normalized);

	if (strcmp(normalized, "let x be the greatest common denominator of y we be go to prove the follow")) {
		fprintf(stderr, "test normalizer -- wrong result: %s\n", normalized);
		free(normalized);
		return 1;
	}

	free(normalized);
	close_normalizer();

	return 0;
}