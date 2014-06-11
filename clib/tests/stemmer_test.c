#include "../stemmer.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


int main(void) {
	init_stemmer();

	char *stemmed;

	morpha_stem("there are numbers", &stemmed);

	if (strcmp(stemmed, "there be number")) {
		fprintf(stderr, "test normalizer -- wrong result: %s\n", stemmed);
		free(stemmed);
		return 1;
	}

	free(stemmed);

	morpha_stem("Let x be the greatest common denominator of y we are going to prove the following", &stemmed);

	if (strcmp(stemmed, "let x be the greatest common denominator of y we be go to prove the follow")) {
		fprintf(stderr, "test normalizer -- wrong result: %s\n", stemmed);
		free(stemmed);
		return 1;
	}

	free(stemmed);
	close_stemmer();

	return 0;
}