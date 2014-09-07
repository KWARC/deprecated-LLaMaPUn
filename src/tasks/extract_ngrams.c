#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ftw.h>
#include <ctype.h>

#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <uthash.h>
#include <pcre.h>

#include "llamapun/tokenizer.h"
#include "llamapun/language_detection.h"
#include "llamapun/utils.h"
#include "llamapun/unicode_normalizer.h"
#include <llamapun/stemmer.h>
#include <llamapun/dnmlib.h>
#include <llamapun/local_paths.h>
#include <llamapun/stopwords.h>


#define FILE_BUFFER_SIZE 2048

struct tmpstuff {
  void *textcat_handle;
  FILE *logfile;
  FILE *statistics;
  struct wordcount *unigramhash;
};

struct tmpstuff *stuff = NULL;


struct wordcount {
  char *string;
  long long counter;
  UT_hash_handle hh;
};


struct bigram {
  char *first;
  char *second;
};

struct bigramcount {
  struct bigram bigram;
  long long counter;
  UT_hash_handle hh;
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
};



/* not sure why, but these can't be passed via tmpstuff (causes segfaults in macros :/ ) */
struct bigramcount *BIGRAM_HASH = NULL;
struct trigramcount *TRIGRAM_HASH = NULL;

int FILE_COUNTER = 0;

/* Copied from gen_TF_IDF.c */
xmlChar *paragraph_xpath = (xmlChar*) "//*[local-name()='section' and @class='ltx_section']//*[local-name()='div' and @class='ltx_para']";
xmlChar *relaxed_paragraph_xpath = (xmlChar*) "//*[local-name()='div' and @class='ltx_para']";

int ngramparse(const char *filename, const struct stat *status, int type) {
  UNUSED(status);
  if (type != FTW_F) return 0;  //not a file
  if (FILE_COUNTER++ > 1000) return 0;   //temporarily treat only a subset
  int NEW_UNIGRAMS_COUNTER = 0;

  const char *regexerror;
  int regexerroroffset;
  pcre *numberregex = pcre_compile("^(('|-|#|\\.)?\\d+[\\d\\.\\)]*[a-z]?[\\d\\.\\)]*)|([a-z]\\.?\\d+[-\\d\\.]*\\)?)$", 0, &regexerror, &regexerroroffset, NULL);
      /* need to catch things like   1.4   12   .004    '65    5.a    1.4.3     a.4    p.5-7   c2   -1  #3    */
      //old (hackier) regex:
     //pcre_compile("^(\\d+([\\.-]?\\d*[a-zA-Z]*)?)|([a-zA-Z]\\.\\d+)$", 0, &regexerror, &regexerroroffset, NULL);
  if (numberregex==NULL) {
    fprintf(stderr, "regex doesn't work (%s) (very fatal)\n", regexerror);
    exit(1);
  }
  pcre_extra *numberregexextra = pcre_study(numberregex, 0, &regexerror);

//  pcre *tmpregex = pcre_compile("\\d", 0, &regexerror, &regexerroroffset, NULL);


  printf("%5d - %s\n", FILE_COUNTER, filename);


/* STEP 1: Load document */
  xmlDoc *document = read_document(filename);

  if (document == NULL) {
    printf("Dismissing document\n");
    fprintf(stuff->logfile, "Dismissing %s (couldn't parse it)\n", filename);
    return 0;
  }

/* STEP 2: Normalize unicode */
  unicode_normalize_dom(document);

/* STEP 3: Get sentences */
  xmlXPathContextPtr xpath_context = xmlXPathNewContext(document);
  if (xpath_context == NULL) {
    fprintf(stderr, "Unable to create xpath context for %s\n", filename);
    fprintf(stuff->logfile, "Unable to create xpath context for %s\n", filename);
    xmlFreeDoc(document);
    return 0;
  }



  /* The following is copied (and slightly adapted) from gen_TF_IDF.c */
  xmlXPathObjectPtr paragraphs_result = xmlXPathEvalExpression(paragraph_xpath,xpath_context);
  if ((paragraphs_result == NULL) || (paragraphs_result->nodesetval == NULL) || (paragraphs_result->nodesetval->nodeNr == 0)) { // Nothing to do if there's no math in the document
    // Clean up this try, before making a second one:
    if (paragraphs_result != NULL) {
      if (paragraphs_result->nodesetval != NULL) {
        free(paragraphs_result->nodesetval->nodeTab);
        free(paragraphs_result->nodesetval); }
      xmlFree(paragraphs_result); }

    // Try the relaxed version: document isn't using LaTeX's \section{}s, maybe it's TeX.
    paragraphs_result = xmlXPathEvalExpression(relaxed_paragraph_xpath,xpath_context);
    if ((paragraphs_result == NULL) || (paragraphs_result->nodesetval == NULL) || (paragraphs_result->nodesetval->nodeNr == 0)) {
      // We're really giving up here, probably empty document
      if (paragraphs_result != NULL) {
        if (paragraphs_result->nodesetval != NULL) {
          free(paragraphs_result->nodesetval->nodeTab);
          free(paragraphs_result->nodesetval); }
        xmlFree(paragraphs_result); }
      xmlXPathFreeContext(xpath_context);
      xmlFreeDoc(document);
      printf("Document seems to be empty - dismissing\n");
      fprintf(stuff->logfile, "Dismissing %s\n", filename);
      return 0; } }
  xmlNodeSetPtr paragraph_nodeset = paragraphs_result->nodesetval;
  int para_index;

  //stuff for ngrams:
  struct wordcount* tmp_wordcount;
  char *penultimate_word;
  char *last_word;
  struct bigram tmpbigram;
  struct bigramcount* tmp_bigramcount;
  struct trigram tmptrigram;
  struct trigramcount* tmp_trigramcount;

  /* Iterate over each paragraph: */
  for (para_index=0; para_index < paragraph_nodeset->nodeNr; para_index++) {
    xmlNodePtr paragraph_node = paragraph_nodeset->nodeTab[para_index];
    // Obtain NLP-friendly plain-text of the paragraph:
    // -- We want to skip tags, as we only are interested in word counts for terms in TF-IDF
    dnmPtr paragraph_dnm = create_DNM(paragraph_node, DNM_NORMALIZE_TAGS | DNM_NO_OFFSETS | DNM_IGNORE_LATEX_NOTES);
    if (paragraph_dnm == NULL) {
      fprintf(stderr, "Couldn't create DNM for paragraph %d in document %s\n",para_index, filename);
      exit(1);
    }
    /* 3. For every paragraph, tokenize sentences: */
    char* paragraph_text = paragraph_dnm->plaintext;
    if (strlen(paragraph_text) < 2) {   //not a real sentence, additionally it would cause errors in tokenizer
      free_DNM(paragraph_dnm);
      continue;
    }
    dnmRanges sentences = tokenize_sentences(paragraph_text);
    /* 4. For every sentence, tokenize words */
    int sentence_index = 0;
    for (sentence_index = 0; sentence_index < sentences.length; sentence_index++) {
      // Obtaining only the content words here
      dnmRanges words = tokenize_words(paragraph_text, sentences.range[sentence_index], 0);
      penultimate_word = NULL;   //start all over again, after sentence boundary
      last_word = NULL;
      int word_index;
      for(word_index=0; word_index<words.length; word_index++) {
        char* word_string = plain_range_to_string(paragraph_text, words.range[word_index]);
        char* word_stem;
        full_morpha_stem(word_string, &word_stem);
        free(word_string);
        // Note: SENNA's tokenization has some features to keep in mind:
        //  multi-symplectic --> "multi-" and "symplectic"
        //  Birkhoff's       --> "birkhoff" and "'s"
        // Add to the document frequency
        //record_word(&DF, word_stem);

        if (!pcre_exec(numberregex, numberregexextra, word_stem, strlen(word_stem), 0, 0, NULL, 0)) {
          free(word_stem);
          word_stem = strdup("numberexpression");
        }

//        if (!pcre_exec(tmpregex, NULL, word_stem, strlen(word_stem), 0, 0, NULL, 0)) {
//          printf("Found: %s\n", word_stem);
//        }

        if (is_stopword(word_stem) || (!isalnum(*word_stem) && *word_stem != '\'')) {
          penultimate_word = NULL;
          last_word = NULL;
          free(word_stem);
          continue;
        }


//THE ACTUAL NGRAM GATHERING
        HASH_FIND_STR(stuff->unigramhash, word_stem, tmp_wordcount);
        if (tmp_wordcount == NULL) { //word hasn't occured yet
          NEW_UNIGRAMS_COUNTER++;
          tmp_wordcount = (struct wordcount*) malloc(sizeof(struct wordcount));
          tmp_wordcount->string = word_stem;
          tmp_wordcount->counter = 1;
          HASH_ADD_KEYPTR(hh, stuff->unigramhash, tmp_wordcount->string, strlen(tmp_wordcount->string), tmp_wordcount);
        } else {
          free(word_stem);
          word_stem = tmp_wordcount->string;
          tmp_wordcount->counter++;
        }

        if (last_word) {
          //do the bigram stuff
          tmpbigram.first = last_word;
          tmpbigram.second = word_stem;
          HASH_FIND(hh, BIGRAM_HASH, (&tmpbigram), sizeof(struct bigram), tmp_bigramcount);
          if (tmp_bigramcount==NULL) {
            tmp_bigramcount = (struct bigramcount*) malloc(sizeof(struct bigramcount));
            tmp_bigramcount->bigram = tmpbigram;
            tmp_bigramcount->counter = 1;
            HASH_ADD(hh, BIGRAM_HASH, bigram, sizeof(struct bigram), tmp_bigramcount);
          } else {
            tmp_bigramcount->counter++;
          }
        }

        if (penultimate_word) {
          //do the trigram stuff
          tmptrigram.first = penultimate_word;
          tmptrigram.second = last_word;
          tmptrigram.third = word_stem;
          HASH_FIND(hh, TRIGRAM_HASH, (&tmptrigram), sizeof(struct trigram), tmp_trigramcount);
          if (tmp_trigramcount == NULL) {
            tmp_trigramcount = (struct trigramcount*) malloc(sizeof(struct trigramcount));
            tmp_trigramcount->trigram = tmptrigram;
            tmp_trigramcount->counter = 1;
            HASH_ADD(hh, TRIGRAM_HASH, trigram, sizeof(struct trigram), tmp_trigramcount);
          } else {
            tmp_trigramcount->counter++;
          }
        }

        //shift words
        penultimate_word = last_word;
        last_word = word_stem;

      }
      free(words.range);
    }
    free(sentences.range);
    free_DNM(paragraph_dnm);
  }
  free(paragraphs_result->nodesetval->nodeTab);
  free(paragraphs_result->nodesetval);
  xmlFree(paragraphs_result);
  xmlXPathFreeContext(xpath_context);
  pcre_free(numberregex);
  if (numberregexextra) {
    pcre_free(numberregexextra);
  }

  fprintf(stuff->statistics, "%s\t%d\n", filename, NEW_UNIGRAMS_COUNTER);

  xmlFreeDoc(document);

  return 0;
}




int main(int argc, char *argv[]) {
  load_stopwords();
  init_stemmer();
  stuff = (struct tmpstuff*) malloc(sizeof(struct tmpstuff));
  stuff->textcat_handle = llamapun_textcat_Init();
  if (!stuff->textcat_handle) {
    fprintf(stderr, "Couldn't load textcat handle (fatal)\n");
    exit(1);
  }
  stuff->logfile = fopen("logfile.txt", "w");
  stuff->statistics = fopen("statistics.txt", "w");
  stuff->unigramhash = NULL;
  /*for bigrams and trigrams global variables are needed :/ */

  char senna_opt_path[FILE_BUFFER_SIZE];
  snprintf(senna_opt_path, FILE_BUFFER_SIZE, "%sthird-party/senna/", LLAMAPUN_ROOT_PATH);
  initialize_tokenizer(senna_opt_path);

  if (argc == 1) {
    ftw(".", ngramparse, 1);  //parse working directory
  } else {
    ftw(argv[1], ngramparse, 1);  //parse directory given by first argument
  }

  free_tokenizer();
  free_stopwords();
  xmlCleanupParser();
  close_stemmer();
  textcat_Done(stuff->textcat_handle);


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
      json_object_new_int(current_b->counter));
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
      json_object_new_int(current_t->counter));
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
      json_object_new_int(current_w->counter));
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
