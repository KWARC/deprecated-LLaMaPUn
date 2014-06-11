#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <uthash.h>
#include <json.h>

#include "stopwords.h"



struct stopword_element *STOPWORDS = NULL;


struct stopword_element {
  char *word;
  UT_hash_handle hh;
};


void read_stopwords(json_object *stopwords_json) {
  /* Read in the list of stopwords as a hash */
  
  int stopwords_count = json_object_array_length(stopwords_json);
  struct stopword_element *stopword_hash = NULL;
  int i;
  for (i=0; i< stopwords_count; i++){
    json_object *word_json = json_object_array_get_idx(stopwords_json, i); /*Getting the array element at position i*/
    const char *stopword = json_object_get_string(word_json);
    stopword_hash = (struct stopword_element*)malloc(sizeof(struct stopword_element));
    stopword_hash->word = strdup(stopword);
    HASH_ADD_KEYPTR( hh, STOPWORDS, stopword_hash->word, strlen(stopword_hash->word), stopword_hash ); }
}

void free_stopwords() {
  /* deallocates the memory */
  struct stopword_element *current, *tmp;
  HASH_ITER(hh, STOPWORDS, current, tmp) {
    HASH_DEL(STOPWORDS,current);
    free(current->word);
    free(current);
  }
}

int is_stopword(const char *word) {
  /* checks whether word is one of the stopwords */
  if (STOPWORDS == NULL) {
    fprintf(stderr, "Stopwords not initialized");
    return 0;
  }
  struct stopword_element* tmp;
  HASH_FIND_STR(STOPWORDS, word, tmp);
  if (tmp == NULL) return 0;
  else return 1;
}
