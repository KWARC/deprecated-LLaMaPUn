#include "llamapun/unicode_normalizer.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


int main(void) {
  char *result;
  char *input = strdup("Poincar\xc3\xa9 and \xc3\xa6");
  normalize_unicode(input, &result);
  if (strcmp(result, "Poincare and ae")) {
    fprintf(stderr, "Error: Expected Poincare, got %s\n", result);
    free(result);
    return 1;
  }
  free(input);
  free(result);
  return 0;
}
