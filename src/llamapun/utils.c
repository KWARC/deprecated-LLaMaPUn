// Some everyday C includes
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <llamapun/utils.h>
#include <llamapun/json_include.h>

char* cortex_stringify_response(json_object* response, size_t *size) {
  const char* string_response = json_object_to_json_string(response);
  *size = strlen(string_response);
  return (char *)string_response; }

json_object* cortex_response_json(char *annotations, char *message, int status) {
  json_object *response = json_object_new_object();
  json_object *json_log = json_object_new_string(message);
  json_object *json_status = json_object_new_int(status);
  json_object *json_annotations = json_object_new_string(annotations);

  json_object_object_add(response,"annotations", json_annotations);
  json_object_object_add(response,"log", json_log);
  json_object_object_add(response,"status", json_status);
  return response; }


char* file_contents(const char* filename) {
  char* content;
  size_t size, size_read;
  FILE* file = fopen(filename,"rb");
  if (file == NULL) {
    fprintf(stderr, "Couldn't open %s\n", filename);
    return NULL;
  }
  fseek(file, 0, SEEK_END);
  size = ftell(file);
  fseek(file, 0, SEEK_SET);
  content = (char *) malloc(sizeof(char) * (size + 1));
  size_read = fread(content, sizeof(char), size, file);
  UNUSED(size_read);
  fclose(file);
  content[size] = '\0';
  return content;
}

json_object* read_json(const char* filename) {
  char* content = file_contents(filename);
  if (content == NULL) {
    fprintf(stderr, "No content at %s\n",filename);
    return NULL;
  }
  json_object* json_content = json_tokener_parse(content);
  free(content);
  return json_content;
}

xmlDocPtr read_document(const char* filename) {
  //read file
  char *content = file_contents(filename);
  size_t size = strlen(content);
  /* the following is copied and adapted from
   https://github.com/KWARC/mws/blob/master/src/crawler/parser/XmlParser.cpp#L65 */
  xmlDocPtr doc;
  // Try as XHTML
  doc = xmlReadMemory(content, size, /* URL = */ "", "UTF-8",
                      XML_PARSE_NOWARNING | XML_PARSE_NOERROR);
  if (doc != NULL) {
    free(content);
    return doc;
  }
  // Try as HTML
  htmlParserCtxtPtr htmlParserCtxt = htmlNewParserCtxt();
  doc = htmlCtxtReadMemory(
        htmlParserCtxt, content, size, /* URL = */ "", "UTF-8",
        HTML_PARSE_RECOVER | HTML_PARSE_NOWARNING | HTML_PARSE_NOERROR);
  htmlFreeParserCtxt(htmlParserCtxt);
  free(content);
  if (doc != NULL) {
    return doc;
  }

  //if nothing works
  fprintf(stderr, "Couldn't parse %s\n", filename);
  return NULL;
}

/* Frequency-count utilities */
void record_word(struct document_frequencies_hash **hash, char *word) {
  struct document_frequencies_hash *w;
  HASH_FIND_STR(*hash, word, w);  /* word already in the hash? */
  if (w==NULL) { // New word
    w = (struct document_frequencies_hash*)malloc(sizeof(struct document_frequencies_hash));
    w->word = strdup(word);
    w->count = 1;
    HASH_ADD_KEYPTR( hh, *hash, w->word, strlen(w->word), w ); }
  else { // Already exists, just increment the counter:
    w->count++; }
  return;}

int word_key_sort(struct document_frequencies_hash *a, struct document_frequencies_hash *b) {
  return strcmp(a->word,b->word);
}
int document_key_sort(struct corpus_frequencies_hash *a, struct corpus_frequencies_hash *b) {
  return strcmp(a->document,b->document);
}
int descending_count_sort(struct document_frequencies_hash *a, struct document_frequencies_hash *b) {
  return a->count < b->count;
}
int descending_score_sort(struct score_hash *a, struct score_hash *b) {
  return a->score < b->score;
}
int ascending_score_sort(struct score_hash *a, struct score_hash *b) {
  return a->score > b->score;
}

struct score_hash* json_to_score_hash(json_object* json) {
  struct score_hash* scores = NULL;
  char *key;
  struct json_object *val;
  struct lh_entry *entry;
  for(entry = json_object_get_object(json)->head;
      ({ if(entry) { key = (char*)entry->k; val = (struct json_object*)entry->v; } ; entry; });
      entry = entry->next ) {
    // Record the corpus size:
    struct score_hash* entry = (struct score_hash*)malloc(sizeof(struct score_hash));
    entry->word = strdup(key);
    entry->score = json_object_get_double(val);
    HASH_ADD_KEYPTR( hh, scores, entry->word, strlen(entry->word), entry );
  }
  return scores;
}

struct document_frequencies_hash* json_to_document_frequencies_hash(json_object* json) {
  struct document_frequencies_hash* frequencies = NULL;
  char *key;
  struct json_object *val;
  struct lh_entry *entry;
  for(entry = json_object_get_object(json)->head;
      ({ if(entry) { key = (char*)entry->k; val = (struct json_object*)entry->v; } ; entry; });
      entry = entry->next ) {
    // Record the corpus size:
    struct document_frequencies_hash* entry = (struct document_frequencies_hash*)malloc(sizeof(struct document_frequencies_hash));
    entry->word = strdup(key);
    entry->count = json_object_get_int(val);
    HASH_ADD_KEYPTR( hh, frequencies, entry->word, strlen(entry->word), entry );
  }
  return frequencies;
}

json_object* document_frequencies_hash_to_json(struct document_frequencies_hash *DF) {
  json_object *DF_json = json_object_new_object();
  struct document_frequencies_hash *d;
  for(d=DF; d != NULL; d = d->hh.next) {
    json_object_object_add(DF_json, d->word, json_object_new_int(d->count));
  }
  return DF_json;
}
json_object* corpus_frequencies_hash_to_json(struct corpus_frequencies_hash *CF) {
  json_object *CF_json = json_object_new_object();
  struct corpus_frequencies_hash *c;
  for(c=CF; c != NULL; c = c->hh.next) {
    json_object* DF_json = document_frequencies_hash_to_json(c->F);
    json_object_object_add(CF_json, c->document, DF_json);
  }
  return CF_json;
}
json_object* score_hash_to_json(struct score_hash *hash) {
  json_object *json = json_object_new_object();
  struct score_hash *d;
  for(d=hash; d != NULL; d = d->hh.next) {
    json_object_object_add(json, d->word, json_object_new_double(d->score));
  }
  return json;
}
json_object* corpus_scores_hash_to_json(struct corpus_scores_hash *hash) {
  json_object *corpus_json = json_object_new_object();
  struct corpus_scores_hash *c;
  for(c=hash; c != NULL; c = c->hh.next) {
    json_object* document_json = score_hash_to_json(c->scores);
    json_object_object_add(corpus_json, c->document, document_json);
  }
  return corpus_json;
}

void free_document_frequencies_hash(struct document_frequencies_hash *hash) {
  struct document_frequencies_hash *s, *tmp;
  HASH_ITER(hh, hash, s, tmp) {
    HASH_DEL(hash, s);
    free(s->word);
    free(s);
  }
  free(hash);
}
void free_corpus_frequencies_hash(struct corpus_frequencies_hash *hash) {
  struct corpus_frequencies_hash *s, *tmp;
  HASH_ITER(hh, hash, s, tmp) {
    HASH_DEL(hash, s);
    free_document_frequencies_hash(s->F);
    free(s->document);
    free(s);
  }
  free(hash);
}
void free_score_hash(struct score_hash *hash) {
  struct score_hash *s, *tmp;
  HASH_ITER(hh, hash, s, tmp) {
    HASH_DEL(hash, s);
    free(s->word);
    free(s);
  }
  free(hash);
}
void free_corpus_scores_hash(struct corpus_scores_hash *hash) {
  struct corpus_scores_hash *s, *tmp;
  HASH_ITER(hh, hash, s, tmp) {
    HASH_DEL(hash, s);
    free_score_hash(s->scores);
    free(s->document);
    free(s);
  }
  free(hash);
}
