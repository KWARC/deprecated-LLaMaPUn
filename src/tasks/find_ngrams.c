#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ftw.h>

#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>


#include "llamapun/tokenizer.h"
#include "llamapun/language_detection.h"
#include "llamapun/utils.h"
#include "llamapun/unicode_normalizer.h"
#include <llamapun/stemmer.h>
#include <llamapun/dnmlib.h>
#include <llamapun/local_paths.h>


struct tmpstuff {
  void *textcat_handle;
  FILE *logfile;

};

struct tmpstuff *stuff = NULL;


/* Copied from gen_TF_IDF.c */
xmlChar *paragraph_xpath = (xmlChar*) "//*[local-name()='section' and @class='ltx_section']//*[local-name()='div' and @class='ltx_para']";
xmlChar *relaxed_paragraph_xpath = (xmlChar*) "//*[local-name()='div' and @class='ltx_para']";

int ngramparse(const char *filename, const struct stat *status, int type) {
  if (type != FTW_F) return 0;  //not a file

  printf("File: %s\n", filename);


/* STEP 1: Load document */
  xmlDoc *document = read_document(filename);

  if (document == NULL) {
    printf("Dismissing document\n");
    fprintf(stuff->logfile, "Dismissing %s\n", filename);
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


  /* Iterate over each paragraph: */
  for (para_index=0; para_index < paragraph_nodeset->nodeNr; para_index++) {
    xmlNodePtr paragraph_node = paragraph_nodeset->nodeTab[para_index];
    // Obtain NLP-friendly plain-text of the paragraph:
    // -- We want to skip tags, as we only are interested in word counts for terms in TF-IDF
    dnmPtr paragraph_dnm = create_DNM(paragraph_node, DNM_SKIP_TAGS);
    if (paragraph_dnm == NULL) {
      fprintf(stderr, "Couldn't create DNM for paragraph %d in document %s\n",para_index, filename);
      exit(1);
    }
    /* 3. For every paragraph, tokenize sentences: */
    char* paragraph_text = paragraph_dnm->plaintext;
    dnmRanges sentences = tokenize_sentences(paragraph_text);
    /* 4. For every sentence, tokenize words */
    int sentence_index = 0;
    for (sentence_index = 0; sentence_index < sentences.length; sentence_index++) {
      // Obtaining only the content words here
      dnmRanges words = tokenize_words(paragraph_text, sentences.range[sentence_index], 0);
      int word_index;
      for(word_index=0; word_index<words.length; word_index++) {
        char* word_string = plain_range_to_string(paragraph_text, words.range[word_index]);
        char* word_stem;
        morpha_stem(word_string, &word_stem);
        /* Ensure stemming is an invariant (tilings -> tiling -> tile -> tile) */
        while (strcmp(word_string, word_stem) != 0) {
          free(word_string);
          word_string = word_stem;
          morpha_stem(word_string, &word_stem);
        }
        free(word_string);
        // Note: SENNA's tokenization has some features to keep in mind:
        //  multi-symplectic --> "multi-" and "symplectic"
        //  Birkhoff's       --> "birkhoff" and "'s"
        // Add to the document frequency
        //record_word(&DF, word_stem);
        printf("%s\n", word_stem);
        free(word_stem);
      }
      printf("=================\n");
      free(words.range);
    }
    free(sentences.range);
    free_DNM(paragraph_dnm);
  }
  free(paragraphs_result->nodesetval->nodeTab);
  free(paragraphs_result->nodesetval);
  xmlFree(paragraphs_result);
  xmlXPathFreeContext(xpath_context);

  xmlFreeDoc(document);


}




int main(int argc, char *argv[]) {
  stuff = (struct tmpstuff*) malloc(sizeof(struct tmpstuff));
  stuff->textcat_handle = llamapun_textcat_Init();
  if (!stuff->textcat_handle) {
    fprintf(stderr, "Couldn't load textcat handle (fatal)\n");
    exit(1);
  }
  stuff->logfile = fopen("logfile.txt", "w");

  char senna_opt_path[512];
  snprintf(senna_opt_path, strlen(senna_opt_path), "%s/third-party/senna/", LLAMAPUN_ROOT_PATH);
  initialize_tokenizer(senna_opt_path);

  if (argc == 1) {
    ftw(".", ngramparse, 1);  //parse working directory
  } else {
    ftw(argv[1], ngramparse, 1);  //parse directory given by first argument
  }


  textcat_Done(stuff->textcat_handle);
  fclose(stuff->logfile);

  free_tokenizer();
  xmlCleanupParser();

  return 0;
}
