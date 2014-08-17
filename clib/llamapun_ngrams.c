// Standard C libraries
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
// Hashes
#include <uthash.h>
// JSON
//#include <json-c/json.h>
#include "jsoninclude.h"
// LLaMaPUn Utils
#include "llamapun_utils.h"
#include "stopwords.h"
#include "stemmer.h"
#include "llamapun_ngrams.h"
#include "unicode_normalizer.h"
#include "old_dnmlib.h"


struct stringcount {
  char *string;
  int counter;
  UT_hash_handle hh;
};


json_object* llamapun_get_ngrams (xmlDocPtr doc) {
  char* log_message;
  if (doc == NULL) {
    fprintf(stderr, "Failed to parse workload!\n");
    log_message = "Failed to parse document.";
    return cortex_response_json("",log_message,-4);
  }

  //initialize everything for counting ngrams
  init_stemmer();
  struct stringcount *unigram_hash = NULL;
  struct stringcount * bigram_hash = NULL;
  struct stringcount *trigram_hash = NULL;

  load_stopwords();

  old_dnmPtr dnm = old_createDNM(doc, DNM_NORMALIZE_MATH | DNM_SKIP_CITE);
  dnmIteratorPtr it_sent = getDnmIterator(dnm, DNM_LEVEL_SENTENCE);

  //loop over sentences
  do {
    int isnumber;
    size_t tmpindex;
    dnmIteratorPtr it_word = getDnmChildrenIterator(it_sent);
    if (it_word == NULL) //no words
      continue;


    size_t sentence_size;
    FILE *word_stream;
    char *word_input_string;
    word_stream = open_memstream (&word_input_string, &sentence_size);

    //loop over words
    do {
      char* word_content = getDnmIteratorContent(it_word);

      //add word_content to word_stream (replace numbers by "[number]")
      isnumber = 1;
      tmpindex = 0;
      while (tmpindex < strlen(word_content)) {
        if (!(isdigit(word_content[tmpindex]) || word_content[tmpindex] == '.')) {
          isnumber = 0;
          break;
        }
        tmpindex++;
      }
      if (isnumber) {
        fprintf(word_stream, "[number]");
      } else {
        fprintf(word_stream, "%s", word_content);
      }
      if (it_word->pos < it_word->end - 1) fprintf(word_stream, " ");   //no trailing space!!

      free(word_content);
    } while (dnmIteratorNext(it_word));

    free(it_word);

    fclose(word_stream);

    //prepare for ngram extraction
    char *stemmed;
    char *tmp1,*tmp2,*tmp;
    normalize_unicode(word_input_string, &tmp);
    morpha_stem(tmp, &stemmed);
    free(tmp);
    char *index = stemmed;
    int foundStopword;
    int foundEnd = 0;
    int nonstopcounter;

    //do the actual ngram extraction (iteratively)
    do {
      //step 1: remove leading stop words
      foundStopword = 1;
      while (foundStopword && !foundEnd) {
        tmp = index;
        while (!(isspace(*tmp) || !*tmp)) tmp++;
        foundEnd = (*tmp == '\0'); 
        *tmp = '\0';
        foundStopword = is_stopword(index);
        if (foundStopword) {
          index = tmp;   //move on
        }
        if (!foundEnd) *tmp = ' ';
        while (isspace(*index)) index++;  //go to first character of new word
      }
  
      //step 2: Count leading "non-stop" words
      nonstopcounter = 0;
      foundStopword = 0;
      if (foundEnd && !foundStopword) nonstopcounter = 1;
      tmp1 = index;
      while (!foundStopword && !foundEnd) {
        tmp2 = tmp1;    //tmp1 is supposed to point to the first letter of the first word that is not yet checked
                  //tmp2 shall point to the first whitespace thereafter
        while (!(isspace(*tmp2) || !*tmp2)) tmp2++;
        foundEnd = (*tmp2 == '\0'); 
        *tmp2 = '\0';
        foundStopword = is_stopword(tmp1);
        if (!foundStopword) {
          nonstopcounter++;
          tmp1 = tmp2;   //move on
        }
        if (!foundEnd) *tmp2 = ' ';
        while (isspace(*tmp1)) tmp1++;  //go to first character of new word
      }
      
      //step 3: process leading "non-stop" words
      tmp2 = index;   //tmp2 is starting position for next iteration
      struct stringcount *tmp_strcount;
      while (nonstopcounter--) {
        index = tmp2;
        tmp = index;
        //unigram
        while (!(isspace(*tmp) || !*tmp)) tmp++;
        *tmp = '\0';
        //add_ngram(index, unigram_hash);

        HASH_FIND_STR(unigram_hash, index, tmp_strcount);
        if (tmp_strcount == NULL) {
          tmp_strcount = (struct stringcount *) malloc(sizeof(struct stringcount));
          tmp_strcount->string = strdup(index);
          tmp_strcount->counter = 1;
          HASH_ADD_KEYPTR(hh, unigram_hash, tmp_strcount->string, strlen(tmp_strcount->string), tmp_strcount);
        } else {    //ngram is already in hash
          tmp_strcount->counter++;
        }

        *tmp = ' ';
        while (isspace(*++tmp));
        tmp2 = tmp;    //next index
        if (nonstopcounter < 1) continue;
        //bigram
        while (!(isspace(*tmp) || !*tmp)) tmp++;
        *tmp = '\0';
        //add_ngram(index, bigram_hash);
        
        HASH_FIND_STR(bigram_hash, index, tmp_strcount);
        if (tmp_strcount == NULL) {
          tmp_strcount = (struct stringcount *) malloc(sizeof(struct stringcount));
          tmp_strcount->string = strdup(index);
          tmp_strcount->counter = 1;
          HASH_ADD_KEYPTR(hh, bigram_hash, tmp_strcount->string, strlen(tmp_strcount->string), tmp_strcount);
        } else {    //ngram is already in hash
          tmp_strcount->counter++;
        }

        *tmp = ' ';
        while (isspace(*++tmp));
        if (nonstopcounter < 2) continue;
        //trigram
        while (!(isspace(*tmp) || !*tmp)) tmp++;
        *tmp = '\0';
        //add_ngram(index, trigram_hash);

        HASH_FIND_STR(trigram_hash, index, tmp_strcount);
        if (tmp_strcount == NULL) {
          tmp_strcount = (struct stringcount *) malloc(sizeof(struct stringcount));
          tmp_strcount->string = strdup(index);
          tmp_strcount->counter = 1;
          HASH_ADD_KEYPTR(hh, trigram_hash, tmp_strcount->string, strlen(tmp_strcount->string), tmp_strcount);
        } else {    //ngram is already in hash
          tmp_strcount->counter++;
        }

        *tmp = ' ';
        while (isspace(*++tmp));
      }
      index = tmp2;

  
    } while (!foundEnd);
    free(word_input_string);
    free(stemmed);

  } while (dnmIteratorNext(it_sent));

  free(it_sent);
  old_freeDNM(dnm);

  close_stemmer();
  free_stopwords();

  //now transfer data to json object
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
    free(current->string);
    free(current);
  }
  HASH_ITER(hh, bigram_hash, current, tmp) {
    json_object_object_add(bigrams, current->string,
          json_object_new_int(current->counter));
    HASH_DEL(bigram_hash, current);
    free(current->string);
    free(current);
  }
  HASH_ITER(hh, trigram_hash, current, tmp) {
    json_object_object_add(trigrams, current->string,
          json_object_new_int(current->counter));
    HASH_DEL(trigram_hash, current);
    free(current->string);
    free(current);
  }

  json_object_object_add(response, "unigrams", unigrams);
  json_object_object_add(response,  "bigrams",  bigrams);
  json_object_object_add(response, "trigrams", trigrams);
  return response;
}
