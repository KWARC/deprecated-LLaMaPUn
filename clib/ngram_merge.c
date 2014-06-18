#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <json.h>
#include <uthash.h>
#include "ngram_merge.h"

char *my_strdup(const char *string) {   //strdup doesn't go along with c99, which is required for json_object_object_foreach
    char *copy = malloc(strlen(string)+1);
    if (copy!=NULL) strcpy(copy, string);
    return copy;
}

struct stringcount {
	char *string;
	int counter;
	UT_hash_handle hh;
};

struct stringcount *unigram_hash = NULL;
struct stringcount  *bigram_hash = NULL;
struct stringcount *trigram_hash = NULL;

void ngram_merge_push(json_object * jobj) {
	json_object* tmp;
	struct stringcount *tmp_strcount;

	//merge unigrams
	{tmp = json_object_object_get(jobj, "unigrams");
		json_object_object_foreach(tmp, key, val) {
			HASH_FIND_STR(unigram_hash, key, tmp_strcount);
			if (tmp_strcount == NULL) {
				tmp_strcount = (struct stringcount *) malloc(sizeof(struct stringcount));
				tmp_strcount->string = my_strdup(key);
				tmp_strcount->counter = json_object_get_int(val);
				HASH_ADD_KEYPTR(hh, unigram_hash, tmp_strcount->string, strlen(tmp_strcount->string), tmp_strcount);
			} else {    //ngram is already in hash
				tmp_strcount->counter += json_object_get_int(val);
			}
		}}

	//merge bigrams
	{tmp = json_object_object_get(jobj, "bigrams");
		json_object_object_foreach(tmp, key, val) {
			HASH_FIND_STR(bigram_hash, key, tmp_strcount);
			if (tmp_strcount == NULL) {
				tmp_strcount = (struct stringcount *) malloc(sizeof(struct stringcount));
				tmp_strcount->string = my_strdup(key);
				tmp_strcount->counter = json_object_get_int(val);
				HASH_ADD_KEYPTR(hh, bigram_hash, tmp_strcount->string, strlen(tmp_strcount->string), tmp_strcount);
			} else {    //ngram is already in hash
				tmp_strcount->counter += json_object_get_int(val);
			}
		}}

	//merge trigrams
	{tmp = json_object_object_get(jobj, "trigrams");
		json_object_object_foreach(tmp, key, val) {
			HASH_FIND_STR(trigram_hash, key, tmp_strcount);
			if (tmp_strcount == NULL) {
				tmp_strcount = (struct stringcount *) malloc(sizeof(struct stringcount));
				tmp_strcount->string = my_strdup(key);
				tmp_strcount->counter = json_object_get_int(val);
				HASH_ADD_KEYPTR(hh, trigram_hash, tmp_strcount->string, strlen(tmp_strcount->string), tmp_strcount);
			} else {    //ngram is already in hash
				tmp_strcount->counter += json_object_get_int(val);
			}
		}}
}



json_object *pull_merged_ngrams() {
	json_object* response = json_object_new_object();
	json_object* unigrams = json_object_new_object();
	json_object*  bigrams = json_object_new_object();
	json_object* trigrams = json_object_new_object();

	struct stringcount *current;
	struct stringcount *tmp;

	HASH_ITER(hh, unigram_hash, current, tmp) {
		json_object_object_add(unigrams, current->string,
		      json_object_new_int(current->counter));
		HASH_DEL(unigram_hash, current);
		free(current);
	}
	HASH_ITER(hh, bigram_hash, current, tmp) {
		json_object_object_add(bigrams, current->string,
		      json_object_new_int(current->counter));
		HASH_DEL(bigram_hash, current);
		free(current);
	}
	HASH_ITER(hh, trigram_hash, current, tmp) {
		json_object_object_add(trigrams, current->string,
		      json_object_new_int(current->counter));
		HASH_DEL(trigram_hash, current);
		free(current);
	}

	json_object_object_add(response, "unigrams", unigrams);
	json_object_object_add(response,  "bigrams",  bigrams);
	json_object_object_add(response, "trigrams", trigrams);
	return response;
}
