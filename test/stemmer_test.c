#include "llamapun/stemmer.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


int main(void) {
  init_stemmer();

  char *stemmed;
  char *test;

  test = strdup("there are numbers");
  morpha_stem(test, &stemmed);
  free(test);

  if (strcmp(stemmed, "there be number")) {
    fprintf(stderr, "test normalizer -- wrong result: %s\n", stemmed);
    free(stemmed);
    return 1;
  }

  free(stemmed);

  test = strdup("Let x be the greatest common denominator of y we are going to prove the following");
  morpha_stem(test, &stemmed);
  free(test);

  if (strcmp(stemmed, "let x be the greatest common denominator of y we be go to prove the follow")) {
    fprintf(stderr, "test normalizer -- wrong result: %s\n", stemmed);
    free(stemmed);
    return 1;
  }

  free(stemmed);

  test = strdup("tilings");
  full_morpha_stem(test, &stemmed);
  free(test);

  if (strcmp(stemmed, "tile")) {
    fprintf(stderr, "test normalizer -- wrong result: %s\n", stemmed);
    free(stemmed);
    return 1;
  }

  free(stemmed);

  close_stemmer();

  return 0;
}
