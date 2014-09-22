#include <pcre.h>
#include <stdio.h>
#include <string.h>

#define numeric_word_pattern "^([\\+\\=0-9][0-9\\+\\=\\.\\-\\/\\,\\(\\)\\']*)|([vix]+\\.)$"
#define OVECCOUNT 30    /* should be a multiple of 3 */
pcre *numeric_regex;

void initialize_word_normalizer() {
  const char* error;
  int error_offset;
  numeric_regex = pcre_compile(numeric_word_pattern, 0, &error, &error_offset, NULL);
}

void normalize_word(char** word) {
  int numeric_vector[OVECCOUNT];
  int numeric_rc = pcre_exec(numeric_regex, NULL, *word, strlen(*word), 0, 0, numeric_vector, OVECCOUNT);
  if (numeric_rc >= 0) { // Numeric, free and replace:
    free(*word);
    *word = strdup("nnumber");
  }
}

void close_word_normalizer() {
  pcre_free(numeric_regex);
}
