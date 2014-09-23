#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ftw.h>

#include <libxml/parser.h>
#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <uthash.h>
#include <pcre.h>

#include "llamapun/tokenizer.h"
#include "llamapun/language_detection.h"
#include "llamapun/utils.h"
#include "llamapun/unicode_normalizer.h"
#include "llamapun/dnmlib.h"
#include <llamapun/stemmer.h>
#include <llamapun/dnmlib.h>
#include <llamapun/local_paths.h>

#include "llamapun/document_loader.h"

static void *TEXT_CAT_HANDLE;
static int (*FUNCTION_FOR_DOCS)(xmlDocPtr, const char *);
static long TRAVERSAL_PARAMETERS;
static FILE *TRAVERSAL_LOG_FILE;


#define CHECK_ALLOC(ptr) {if (ptr==NULL) {fprintf(stderr, "Couldn't allocate memory, exiting\n"); exit(1);}}

#define POSSIBLE_RESIZE(ptr, index, oldsizeptr, newsize, type) \
    {if (index >= *oldsizeptr) {\
      ptr = (type*)realloc(ptr, newsize); *oldsizeptr=newsize; CHECK_ALLOC(ptr); }}

void init_document_loader() {
  TEXT_CAT_HANDLE = llamapun_textcat_Init();
  if (!TEXT_CAT_HANDLE) {
    fprintf(stderr, "Couldn't load textcat handle (fatal)\n");
    exit(1);
  }
  init_stemmer();
  char senna_opt_path[2048];
  snprintf(senna_opt_path, sizeof(senna_opt_path), "%sthird-party/senna/", LLAMAPUN_ROOT_PATH);
  initialize_tokenizer(senna_opt_path);
}

void close_document_loader() {
  textcat_Done(TEXT_CAT_HANDLE);
  xmlCleanupParser();
  free_tokenizer();
  close_stemmer();
}

int read_doc_and_call_function(const char *filename, const struct stat *status, int type) {
  UNUSED(status);
  if (type != FTW_F) return 0;   //not a file
  xmlDoc *document = read_document(filename);

  //if desired, check language or normalize unicode
  if (TRAVERSAL_PARAMETERS & LOADER_CHECK_LANGUAGE) {
    dnmPtr dnm = create_DNM(xmlDocGetRootElement(document), DNM_SKIP_TAGS);
    if (dnm == NULL) {
      fprintf(stderr, "Couldn't create DNM of %s (fatal)\n", filename);
      exit(1);
    }
    if (!text_is_english(TEXT_CAT_HANDLE, dnm->plaintext, dnm->size_plaintext)) {  //if it isn't primarily english
      fprintf(TRAVERSAL_LOG_FILE, "Dismissing %s (appears not to be in English)\n", filename);
      xmlFreeDoc(document);
      return 0;
    }
    free_DNM(dnm);
  }
  if (TRAVERSAL_PARAMETERS & LOADER_NORMALIZE_UNICODE)
    unicode_normalize_dom(document);

  //call the function passed by the user
  int ret_val = FUNCTION_FOR_DOCS(document, filename);

  xmlFreeDoc(document);
  return ret_val;
}

void process_documents_in_directory(int (*function)(xmlDocPtr, const char *), char *dir, long parameters, FILE *logfile) {
  FUNCTION_FOR_DOCS = function;
  TRAVERSAL_LOG_FILE = logfile;
  TRAVERSAL_PARAMETERS = parameters;
  init_document_loader();
  ftw(dir, read_doc_and_call_function, 12);   //12 is the depth (gets slower if exceeded)
  close_document_loader();
  TRAVERSAL_LOG_FILE = stderr;
}

int with_words_at_xpath(void (*function)(char *[], size_t), xmlDocPtr document, const char * xpath, FILE *logfile, long parameters, long dnm_parameters) {
  //if desired, create regex for number expressions
  pcre *numberregex = NULL;
  pcre_extra *numberregexextra = NULL;
  const char *regexerror;
  int regexerroroffset;
  if (parameters & WORDS_NORMALIZE_NUMBERS) {
    /* need to catch things like   1.4   12   .004    '65    5.a    1.4.3     a.4    p.5-7   c2   -1  #3    */
    numberregex = pcre_compile("^(('|-|#|\\.)?\\d+[\\d\\.\\)]*[a-z]?[\\d\\.\\)]*)|([a-z]\\.?\\d+[-\\d\\.]*\\)?)$", 0, &regexerror, &regexerroroffset, NULL);
    if (numberregex == NULL) fprintf(logfile, "regex for numbers doesn't work (%s)\n", regexerror);
    numberregexextra = pcre_study(numberregex, 0, &regexerror);
  }

  //create xpath context
  xmlXPathContextPtr xpath_context = xmlXPathNewContext(document);
  if (xpath_context == NULL) {
    fprintf(logfile, "Unable to create xpath context\n");
    xmlFreeDoc(document);
    return 0;
  }

  //evaluate xpath
  xmlXPathObjectPtr xpath_result = xmlXPathEvalExpression((xmlChar *)xpath,xpath_context);
  if ((xpath_result == NULL) || (xpath_result->nodesetval == NULL) || (xpath_result->nodesetval->nodeNr == 0)) { //didn't find anything
    // Clean up
    if (xpath_result != NULL) {
      if (xpath_result->nodesetval != NULL) {
        free(xpath_result->nodesetval->nodeTab);
        free(xpath_result->nodesetval);
      }
      xmlFree(xpath_result);
    }
    //function call should be done anyway
    function(NULL, 0);
    return 0;   //return false, stating that no nodes where found
  }

  //prepare array for words
  size_t word_array_allocated = 2048;
  char ** word_array = (char **) malloc(word_array_allocated * sizeof(char *));
  size_t word_array_index = 0;

  //iterate over results
  xmlNodeSetPtr nodeset = xpath_result->nodesetval;
  int index;
  for (index=0; index < nodeset->nodeNr; index++) {
    xmlNodePtr current_node = nodeset->nodeTab[index];
    // Obtain NLP-friendly plain-text of the paragraph:
    dnmPtr current_dnm = create_DNM(current_node, dnm_parameters);
    if (current_dnm == NULL) {
      fprintf(stderr, "Couldn't create DNM for xpath %s at position %d in document (fatal)\n", xpath, index);
      exit(1);
    }
    
    //tokenize sentences
    char* plaintext = current_dnm->plaintext;
    if (strlen(plaintext) < 2) {   //not a real sentence, additionally it would cause errors in tokenizer
      free_DNM(current_dnm);
      continue;
    }

    dnmRanges sentences = tokenize_sentences(plaintext);

    // tokenize words
    int sentence_index = 0;
    for (sentence_index = 0; sentence_index < sentences.length; sentence_index++) {
      // Obtaining only the content words here
      dnmRanges words = tokenize_words(plaintext, sentences.range[sentence_index], 0);
      int word_index;
      for(word_index=0; word_index<words.length; word_index++) {
        char* word_string = plain_range_to_string(plaintext, words.range[word_index]);

        if (parameters & WORDS_NORMALIZE_NUMBERS && !pcre_exec(numberregex, numberregexextra, word_string, strlen(word_string), 0, 0, NULL, 0)) {
          free(word_string);
          word_string = strdup("numberexpression");
        }

        if (parameters & WORDS_STEM_WORDS) {
          char* word_stem;
          full_morpha_stem(word_string, &word_stem);
          free(word_string);
          word_string = word_stem;
        }
        POSSIBLE_RESIZE(word_array, word_array_index, &word_array_allocated, word_array_allocated*2, char *);
        word_array[word_array_index++] = word_string;
      }
      free(words.range);
      if (parameters&WORDS_MARK_END_OF_SENTENCE) {
        POSSIBLE_RESIZE(word_array, word_array_index, &word_array_allocated, word_array_allocated*2, char *);
        word_array[word_array_index++] = strdup("endofsentence");
      }
    }
    free(sentences.range);
    free_DNM(current_dnm);

    function(word_array, word_array_index);
    //free memory
    size_t i = 0;
    while (i < word_array_index) free(word_array[i++]);
    word_array_index = 0;
  }
  free(word_array);
  //and clean up
  free(xpath_result->nodesetval->nodeTab);
  free(xpath_result->nodesetval);
  xmlFree(xpath_result);
  xmlXPathFreeContext(xpath_context);
  if (parameters & WORDS_NORMALIZE_NUMBERS) {
    pcre_free(numberregex);
    if (numberregexextra) {
      pcre_free(numberregexextra);
    }
  }
  return 1;   //everything went well
}
