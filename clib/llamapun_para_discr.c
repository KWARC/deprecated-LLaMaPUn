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
#include "llamapun_ngrams.h"
#include "dnmlib.h"


#define NUMBER_OF_THM_TYPES 6

struct word_count {
  char *word;
  int counters[NUMBER_OF_THM_TYPES];
  UT_hash_handle hh;
};


json_object* llamapun_para_discr_get_bags (xmlDocPtr doc) {
  char* log_message;
  if (doc == NULL) {
    fprintf(stderr, "Failed to parse workload!\n");
    log_message = "Failed to parse document.";
    return cortex_response_json("",log_message,-4);
  }

  struct word_count *bag_hash = NULL;
  struct word_count *tmp_word_count;
  char *word;
  int para_type;
  int i;

  dnmPtr dnm = createDNM(doc, DNM_NORMALIZE_MATH | DNM_SKIP_CITE);
  dnmIteratorPtr it_para = getDnmIterator(dnm, DNM_LEVEL_PARA);

  //loop over paragraphs
  do {;
    dnmIteratorPtr it_sent = getDnmChildrenIterator(it_para);
    if (it_sent == NULL) //no sentences
      continue;

    //determine paragraph type
    if (dnmIteratorHasAnnotationInherited(it_para, "ltx_theorem_thm") || \
		dnmIteratorHasAnnotationInherited(it_para, "ltx_theorem_theorem")) 
      para_type = 0;
    else if (dnmIteratorHasAnnotationInherited(it_para, "ltx_theorem_lem") || \
		     dnmIteratorHasAnnotationInherited(it_para, "ltx_theorem_lemma"))
	  para_type = 1;
	else if (dnmIteratorHasAnnotationInherited(it_para, "ltx_theorem_cor") || \
		     dnmIteratorHasAnnotationInherited(it_para, "ltx_theorem_corrolary"))
	  para_type = 2;
	else if (dnmIteratorHasAnnotationInherited(it_para, "ltx_theorem_proof"))
	  para_type = 3;
	else if (dnmIteratorHasAnnotationInherited(it_para, "ltx_theorem_def") || \
		     dnmIteratorHasAnnotationInherited(it_para, "ltx_theorem_definition"))
	  para_type = 4;
	else if (dnmIteratorHasAnnotationInherited(it_para, "ltx_theorem_note") || \
		     dnmIteratorHasAnnotationInherited(it_para, "ltx_theorem_rmk") || \
			 dnmIteratorHasAnnotationInherited(it_para, "ltx_theorem_remakr") || \
			 dnmIteratorHasAnnotationInherited(it_para, "ltx_theorem_rem"))
	  para_type = 5;
    else
      para_type = -1;    //no known paragraph type

    if (para_type < 0) {
      free(it_sent);
      continue;
    }

    //loop over sentences
    do {
      dnmIteratorPtr it_word = getDnmChildrenIterator(it_sent);


      if (it_word == NULL) //no words
        continue;

      //loop over words
      do {
        word = getDnmIteratorContent(it_word);

        HASH_FIND_STR(bag_hash, word, tmp_word_count);
        if (tmp_word_count == NULL) {
          tmp_word_count = (struct word_count *) malloc(sizeof(struct word_count));
          tmp_word_count->word = strdup(word);
          //set counts to 0
          for (i = 0; i < NUMBER_OF_THM_TYPES; i++) {
            tmp_word_count->counters[i] = 0;
          }
          HASH_ADD_KEYPTR(hh,bag_hash, tmp_word_count->word, strlen(tmp_word_count->word), tmp_word_count);
        }
        //increment corresponding counter
        tmp_word_count->counters[para_type]++;

        free(word);

      } while (dnmIteratorNext(it_word));

      free(it_word);

    } while (dnmIteratorNext(it_sent));

    free(it_sent);

  } while (dnmIteratorNext(it_para));

  free(it_para);
  freeDNM(dnm);

    
  //now transfer data to json object
  json_object* response = json_object_new_object();

  struct word_count *current;

  HASH_ITER(hh, bag_hash, current, tmp_word_count) {
    json_object *int_array = json_object_new_array();
    //put counters into array
    for (i=0; i<NUMBER_OF_THM_TYPES; i++) {
      json_object_array_add(int_array, json_object_new_int(current->counters[i]));
    }
    json_object_object_add(response, current->word, int_array);
    HASH_DEL(bag_hash, current);
    free(current->word);
    free(current);
  }

  return response;
}
