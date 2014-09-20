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
}

void close_document_loader() {
  textcat_Done(TEXT_CAT_HANDLE);
  xmlCleanupParser();
}

int read_doc_and_call_function(const char *filename, const struct stat *status, int type) {
  UNUSED(status);
  if (type != FTW_F) return 0;   //not a file
  xmlDoc *document = read_document(filename);

  //if desired, check language or normalize unicode
  if (TRAVERSAL_PARAMETERS & DOC_CHECK_LANGUAGE) {
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
  if (TRAVERSAL_PARAMETERS & DOC_NORMALIZE_UNICODE)
    unicode_normalize_dom(document);

  //call the function passed by the user
  int ret_val = FUNCTION_FOR_DOCS(document, filename);

  xmlFreeDoc(document);
  return ret_val;
}

void traverse_docs_in_dir(char *dir, int (*function)(xmlDocPtr, const char *), long parameters, FILE *logfile) {
  FUNCTION_FOR_DOCS = function;
  TRAVERSAL_LOG_FILE = logfile;
  TRAVERSAL_PARAMETERS = parameters;
  init_document_loader();
  ftw(dir, read_doc_and_call_function, 12);   //12 is the depth (gets slower if exceeded)
  close_document_loader();
  TRAVERSAL_LOG_FILE = stderr;
}
