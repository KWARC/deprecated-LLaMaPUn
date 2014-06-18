#ifndef NGRAM_MERGE_H
#define NGRAM_MERGE_H
#include <json.h>

void ngram_merge_push(json_object * jobj);
json_object *pull_merged_ngrams();
#endif