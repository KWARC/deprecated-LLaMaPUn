#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>


#include "llamapun/stopwords.h"
#include "llamapun/document_loader.h"


#define FILE_BUFFER_SIZE 2048

struct tmpstuff {
  FILE *logfile;
  FILE *statistics;
  struct wordcount *unigramhash;
};

struct tmpstuff *stuff = NULL;


struct wordcount {
  char *string;
  long long counter;
  UT_hash_handle hh;
  unsigned long lastOccurence;
};


struct bigram {
  char *first;
  char *second;
};

struct bigramcount {
  struct bigram bigram;
  long long counter;
  UT_hash_handle hh;
  unsigned long lastOccurence;
};

struct trigram {
  char *first;
  char *second;
  char *third;
};

struct trigramcount {
  struct trigram trigram;
  long long counter;
  UT_hash_handle hh;
  unsigned long lastOccurence;
};



/* not sure why, but these can't be passed via tmpstuff (causes segfaults in macros :/ ) */
struct bigramcount *BIGRAM_HASH = NULL;
struct trigramcount *TRIGRAM_HASH = NULL;

unsigned long FILE_COUNTER = 0;

/* Copied from gen_TF_IDF.c */
char *paragraph_xpath = "//*[local-name()='section' and @class='ltx_section']//*[local-name()='div' and @class='ltx_para']";
char *relaxed_paragraph_xpath = "//*[local-name()='div' and @class='ltx_para']";




void ngram_parse2(char *words[], size_t number, xmlNodePtr node) {
  (void)node;
  struct wordcount* tmp_wordcount;
  char *penultimate_word = NULL;
  char *last_word = NULL;
  char *current_word = NULL;
  struct bigram tmpbigram;
  struct bigramcount* tmp_bigramcount;
  struct trigram tmptrigram;
  struct trigramcount* tmp_trigramcount;


  size_t i;
  for (i=0; i < number; i++) {
    current_word = words[i];
    if (/*is_stopword(current_word) ||*/ !strcmp(current_word, "endofsentence") || (!isalnum(*current_word) && *current_word != '\'')) {
      penultimate_word = NULL;
      last_word = NULL;
      continue;
    }

    HASH_FIND_STR(stuff->unigramhash, current_word, tmp_wordcount);
    if (tmp_wordcount == NULL) { //word hasn't occured yet
      tmp_wordcount = (struct wordcount*) malloc(sizeof(struct wordcount));
      tmp_wordcount->string = strdup(current_word);
      current_word = tmp_wordcount->string;
      tmp_wordcount->counter = 1;
	  tmp_wordcount->lastOccurence = FILE_COUNTER;
      HASH_ADD_KEYPTR(hh, stuff->unigramhash, tmp_wordcount->string, strlen(tmp_wordcount->string), tmp_wordcount);
    } else {
      current_word = tmp_wordcount->string;
      if (tmp_wordcount->lastOccurence != FILE_COUNTER) tmp_wordcount->counter++;
	  tmp_wordcount->lastOccurence = FILE_COUNTER;
    }

    if (last_word) {
      //do the bigram stuff
      tmpbigram.first = last_word;
      tmpbigram.second = current_word;
      HASH_FIND(hh, BIGRAM_HASH, (&tmpbigram), sizeof(struct bigram), tmp_bigramcount);
      if (tmp_bigramcount==NULL) {
        tmp_bigramcount = (struct bigramcount*) malloc(sizeof(struct bigramcount));
        tmp_bigramcount->bigram = tmpbigram;
        tmp_bigramcount->counter = 1;
		tmp_bigramcount->lastOccurence = FILE_COUNTER;
        HASH_ADD(hh, BIGRAM_HASH, bigram, sizeof(struct bigram), tmp_bigramcount);
      } else {
         if (tmp_bigramcount->lastOccurence != FILE_COUNTER) tmp_bigramcount->counter++;
		 tmp_bigramcount->lastOccurence = FILE_COUNTER;
      }
    }

    if (penultimate_word) {
      //do the trigram stuff
      tmptrigram.first = penultimate_word;
      tmptrigram.second = last_word;
      tmptrigram.third = current_word;
      HASH_FIND(hh, TRIGRAM_HASH, (&tmptrigram), sizeof(struct trigram), tmp_trigramcount);
      if (tmp_trigramcount == NULL) {
        tmp_trigramcount = (struct trigramcount*) malloc(sizeof(struct trigramcount));
        tmp_trigramcount->trigram = tmptrigram;
        tmp_trigramcount->counter = 1;
		tmp_trigramcount->lastOccurence = FILE_COUNTER;
        HASH_ADD(hh, TRIGRAM_HASH, trigram, sizeof(struct trigram), tmp_trigramcount);
      } else {
        if (tmp_trigramcount->lastOccurence != FILE_COUNTER) tmp_trigramcount->counter++;
	    tmp_trigramcount->lastOccurence = FILE_COUNTER;
      }
    }

    //shift words
    penultimate_word = last_word;
    last_word = current_word;
  }

  //fprintf(stuff->statistics, "%s\t%d\n", filename, NEW_UNIGRAMS_COUNTER);
}


int ngramparse(xmlDocPtr document, const char *filename) {
  //if (FILE_COUNTER++ > 9) return 1;   //temporarily treat only a subset

  printf("%5ld - %s\n", FILE_COUNTER++, filename);

  int b = with_words_at_xpath(ngram_parse2, document, paragraph_xpath, stuff->logfile,
                WORDS_NORMALIZE_WORDS | WORDS_STEM_WORDS | WORDS_MARK_END_OF_SENTENCE,
                DNM_NORMALIZE_TAGS | DNM_NO_OFFSETS | DNM_IGNORE_LATEX_NOTES);
  if (!b) {   //if xpath doesn't correspond to any node, try relaxed version
    b = with_words_at_xpath(ngram_parse2, document, relaxed_paragraph_xpath, stuff->logfile,
              WORDS_NORMALIZE_WORDS | WORDS_STEM_WORDS | WORDS_MARK_END_OF_SENTENCE,
              DNM_NORMALIZE_TAGS | DNM_NO_OFFSETS | DNM_IGNORE_LATEX_NOTES);
    if (!b) {
      fprintf(stuff->logfile, "Found no paragraphs in %s\n", filename);
    }
  }

  return 0;
}


int main(int argc, char *argv[]) {
  /*for bigrams and trigrams global variables are needed :/ */
  stuff = (struct tmpstuff*) malloc(sizeof(struct tmpstuff));
  stuff->logfile = fopen("logfile.txt", "w");
  stuff->statistics = fopen("statistics.txt", "w");
  stuff->unigramhash = NULL;

  load_stopwords();

  if (argc == 1) {
    //ftw(".", ngramparse, 1);  //parse working directory
    process_documents_in_directory(ngramparse, ".", LOADER_NORMALIZE_UNICODE, stuff->logfile);
  } else {
    //ftw(argv[1], ngramparse, 1);  //parse directory given by first argument
    process_documents_in_directory(ngramparse, argv[1], LOADER_NORMALIZE_UNICODE, stuff->logfile);
  }

  free_stopwords();

  printf("\nSAVE RESULTS\n============\n");
  struct wordcount *current_w, *tmp_w;
  struct bigramcount *current_b, *tmp_b;
  struct trigramcount *current_t, *tmp_t;
  char key[4096]; //don't be stingy here

  printf(" - bigrams\n");    //do bigrams before trigrams, expecting that the JSON stuff will require less memory, so we have more for trigrams left
                             //(unigrams have to be last, cause they point to the actual strings)

  json_object* bigrams = json_object_new_object();

  HASH_ITER(hh, BIGRAM_HASH, current_b, tmp_b) {
    //printf("%s:   %d\n", current_b->string, current_b->counter);
    snprintf(key, sizeof(key), "%s %s", current_b->bigram.first, current_b->bigram.second);
    json_object_object_add(bigrams, key,
      json_object_new_double(1+log2(FILE_COUNTER/(1+current_b->counter))));
    HASH_DEL(BIGRAM_HASH, current_b);
    free(current_b);
  }

  FILE *f = fopen("bigrams.txt", "w");
  fprintf(f, "%s", json_object_to_json_string(bigrams));
  fclose(f);

  json_object_put(bigrams);  //free memory



  printf(" - trigrams\n");

  json_object* trigrams = json_object_new_object();

  HASH_ITER(hh, TRIGRAM_HASH, current_t, tmp_t) {
    //printf("%s:   %d\n", current_t->string, current_t->counter);
    snprintf(key, sizeof(key), "%s %s %s", current_t->trigram.first, current_t->trigram.second, current_t->trigram.third);
    json_object_object_add(trigrams, key,
      json_object_new_double(1+log2(FILE_COUNTER/(1+current_t->counter))));
    HASH_DEL(TRIGRAM_HASH, current_t);
    free(current_t);
  }

  f = fopen("trigrams.txt", "w");
  fprintf(f, "%s", json_object_to_json_string(trigrams));
  fclose(f);

  json_object_put(trigrams);




  printf(" - unigrams\n");

  json_object* unigrams = json_object_new_object();

  HASH_ITER(hh, stuff->unigramhash, current_w, tmp_w) {
    //printf("%s:   %d\n", current_w->string, current_w->counter);
    json_object_object_add(unigrams, current_w->string,
      json_object_new_double(1+log2(FILE_COUNTER/(1.0+current_w->counter))));
    HASH_DEL(stuff->unigramhash, current_w);
    free(current_w->string);
    free(current_w);
  }

  f = fopen("unigrams.txt", "w");
  fprintf(f, "%s", json_object_to_json_string(unigrams));
  fclose(f); 

  json_object_put(unigrams);

  fclose(stuff->logfile);
  fclose(stuff->statistics);

  free(stuff);

  printf("Done :)\n");

  return 0;
}
