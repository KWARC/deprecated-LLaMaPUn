/* CorTeX utility functions and constants */


#ifndef LLAMAPUN_UTILS_H
#define LLAMAPUN_UTILS_H

#include <uthash.h>
#include <llamapun/json_include.h>
#include <libxml/parser.h>
#include <libxml/HTMLparser.h>

char* cortex_stringify_response(json_object* response, size_t *size);

json_object* cortex_response_json(char *annotations, char *message, int status);

/* reads a document and returns the DOM

  Tries both, interpreting the files as XHTML, and interpreting it as HTML5
  @param filename the file name
  @retval a pointer to the DOM (NULL if reading/parsing wasn't successful)
*/
xmlDocPtr read_document(const char* filename);

/* Common datastructures for frequency methods */
struct document_frequencies_hash {
  char *word; /* we'll use this field as the key */
  int count;
  UT_hash_handle hh; /* makes this structure hashable */
};
struct corpus_frequencies_hash {
  char* document;
  struct document_frequencies_hash *F;
  UT_hash_handle hh; /* makes this structure hashable */
};
struct score_hash {
  char* word;
  double score;
  UT_hash_handle hh; /* makes this structure hashable */
};
struct corpus_scores_hash {
  char* document;
  struct score_hash *scores;
  UT_hash_handle hh; /* makes this structure hashable */
};

void record_word(struct document_frequencies_hash **hash, char *word);
int word_key_sort(struct document_frequencies_hash *a, struct document_frequencies_hash *b);
int document_key_sort(struct corpus_frequencies_hash *a, struct corpus_frequencies_hash *b);
int descending_count_sort(struct document_frequencies_hash *a, struct document_frequencies_hash *b);
int descending_tf_idf_sort(struct score_hash *a, struct score_hash *b);
int descending_score_sort(struct score_hash *a, struct score_hash *b);

json_object* document_frequencies_hash_to_json(struct document_frequencies_hash *DF);
json_object* corpus_frequencies_hash_to_json(struct corpus_frequencies_hash *CF);
json_object* score_hash_to_json(struct score_hash *hash);
json_object* corpus_scores_hash_to_json(struct corpus_scores_hash *hash);

void free_document_frequencies_hash(struct document_frequencies_hash *DF);
void free_corpus_frequencies_hash(struct corpus_frequencies_hash *CF);
void free_scores_hash(struct score_hash *SH);
void free_corpus_scores_hash(struct corpus_scores_hash *CTF);


#endif
