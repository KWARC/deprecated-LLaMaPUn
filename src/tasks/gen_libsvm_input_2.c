#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ftw.h>
#include <uthash.h>
#include <math.h>
#include <sys/stat.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <assert.h>

#include <llamapun/local_paths.h>
#include <llamapun/json_include.h>
#include <llamapun/utils.h>
#include <llamapun/document_loader.h>
#include <llamapun/crf.h>
#include <llamapun/word_normalizer.h>
#include <llamapun/tokenizer.h>
#include <llamapun/stemmer.h>

#define FILENAME_BUFF_SIZE 2048

xmlChar *paragraph_xpath = (xmlChar*)
    "//*[local-name()='section' and @class='ltx_section']//*[local-name()='div' and @class='ltx_para']";
xmlChar *relaxed_paragraph_xpath = (xmlChar*) "//*[local-name()='div' and @class='ltx_para']";


int is_definition(xmlNode * n) {
  if (!xmlStrEqual(n->name, BAD_CAST "div")) return 0;
  xmlAttr *attr;
  char *class_value;
  int retval = 0;
  for (attr = n->properties; attr != NULL; attr = attr->next) {
    if (xmlStrEqual(attr->name, BAD_CAST "class")) {
      class_value = (char*) xmlNodeGetContent(attr->children);
      if(strstr(class_value, "ltx_theorem_def") != NULL) {
        retval = 1; }
      xmlFree(class_value);
      return retval;
    }
  }
  return 0;   // has no class
}

// Global IDF:
struct score_hash *idf;
struct document_frequencies_hash* word_to_bin = NULL;
// File handle for the svm input
FILE* svm_input_file;


int process_file(xmlDocPtr document, const char *filename) {
  if (document == NULL) {
    fprintf(stderr, "Couldn't parse %s\n", filename);
  }
  /* 1. Normalize Unicode */
  //unicode_normalize_dom(document);   //normalized by document loader already
  /* 2. Select paragraphs */
  xmlXPathContextPtr xpath_context = xmlXPathNewContext(document);
  if(xpath_context == NULL) {
    fprintf(stderr,"Error: unable to create new XPath context for %s\n",filename);
    xmlFreeDoc(document);
    exit(1); }
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
      fprintf(stderr, "Couldn't find paragraphs in %s\n", filename);
      return 0; } }
  xmlNodeSetPtr paragraph_nodeset = paragraphs_result->nodesetval;
  int para_index;

  /* ------------------------------------------------- */
  /* ------------------------------------------------- */
  /* First iteration: compute TF frequencies           */
  struct document_frequencies_hash *DF = NULL;
  for (para_index=0; para_index < paragraph_nodeset->nodeNr; para_index++) {
    xmlNodePtr paragraph_node = paragraph_nodeset->nodeTab[para_index];
    // Obtain NLP-friendly plain-text of the paragraph:
    // -- We want to skip tags, as we only are interested in word counts for terms in TF-IDF
    dnmPtr paragraph_dnm = create_DNM(paragraph_node, DNM_SKIP_TAGS | DNM_IGNORE_LATEX_NOTES);
    if (paragraph_dnm == NULL) {
      fprintf(stderr, "Couldn't create DNM for paragraph %d in document %s\n",para_index, filename);
      exit(1);
    }
    /* For every paragraph, tokenize sentences: */
    char* paragraph_text = paragraph_dnm->plaintext;
    dnmRanges sentences = tokenize_sentences(paragraph_text);
    /* For every sentence, tokenize words */
    int sentence_index = 0;
    for (sentence_index = 0; sentence_index < sentences.length; sentence_index++) {
      //fprintf(stderr,"---------\n");
      //dill_morpha_stll_morpha_stsplay_range(paragraph_text, sentences.range[sentence_index]);
      // Obtaining only the content words here, disregard stopwords and punctuation
      dnmRanges words = tokenize_words(paragraph_text, sentences.range[sentence_index],
                                       TOKENIZER_ALPHA_ONLY | TOKENIZER_FILTER_STOPWORDS);
      int word_index;
      for(word_index=0; word_index<words.length; word_index++) {
        char* word_string = plain_range_to_string(paragraph_text, words.range[word_index]);
        char* word_stem;
        normalize_word(&word_string);
        full_morpha_stem(word_string, &word_stem);
        //fprintf(stderr,"%s --> %s\n",word_string,word_stem);
        free(word_string);
        record_word(&DF,word_stem);
        free(word_stem);
      }
      free(words.range);
    }
    free(sentences.range);
    free_DNM(paragraph_dnm);
  }
  struct document_frequencies_hash *word_entry;
  int doc_max_count = 0;
  for(word_entry=DF; word_entry != NULL; word_entry = word_entry->hh.next) {
    if (doc_max_count < word_entry->count) {
      doc_max_count = word_entry->count;
    }
  }

  /* ------------------------------------------------- */
  /* ------------------------------------------------- */
  /* SECOND iteration compute scores and emit vectors: */
  for (para_index=0; para_index < paragraph_nodeset->nodeNr; para_index++) {
    // Every paragraph is mapped to a vector of bins:
    double bins[20000] = { 0 };
    int i=0;
    bool recorded_bin = false;
    for (i=0; i<20000; i++) bins[i]=0;
    xmlNodePtr paragraph_node = paragraph_nodeset->nodeTab[para_index];
    // Obtain NLP-friendly plain-text of the paragraph:
    // -- We want to skip tags, as we only are interested in word counts for terms in TF-IDF
    dnmPtr paragraph_dnm = create_DNM(paragraph_node, DNM_SKIP_TAGS | DNM_IGNORE_LATEX_NOTES);
    if (paragraph_dnm == NULL) {
      fprintf(stderr, "Couldn't create DNM for paragraph %d in document %s\n",para_index, filename);
    }
    /* And a counter for the words */
    unsigned int paragraph_word_count = 0;
    /* For every paragraph, tokenize sentences: */
    char* paragraph_text = paragraph_dnm->plaintext;
    dnmRanges sentences = tokenize_sentences(paragraph_text);
    /* For every sentence, tokenize words */
    int sentence_index = 0;
    double tfc_normalization_divisor = 1.0;    //determine divisor for tf_idf normalization

    for (sentence_index = 0; sentence_index < sentences.length; sentence_index++) {
      // Obtaining only the content words here, disregard stopwords and punctuation
      dnmRanges words = tokenize_words(paragraph_text, sentences.range[sentence_index],
                                       TOKENIZER_ALPHA_ONLY | TOKENIZER_FILTER_STOPWORDS);
      int word_index;


      for(word_index=0; word_index<words.length; word_index++) {
        paragraph_word_count++;
        char* word_string = plain_range_to_string(paragraph_text, words.range[word_index]);
        char* word_stem;
        full_morpha_stem(word_string, &word_stem);
        free(word_string);

        // Skip unless we have a bin (i.e. unless the word is common):
        struct document_frequencies_hash* bin_entry;
        HASH_FIND_STR(word_to_bin, word_stem, bin_entry);
        if (bin_entry != NULL) {
          int bin = bin_entry->count;
          // We're using bins, numbered by the word stem and valued with the TFxIDF scores
          // Compute the TF:
          HASH_FIND_STR(DF, word_stem, word_entry);
          double word_tf = 0.5 + (0.5 * word_entry->count)/doc_max_count;
          // Lookup the IDF:
          struct score_hash* idf_entry;
          HASH_FIND_STR(idf, word_stem, idf_entry);
          double word_idf = 13;
          if(idf_entry!=NULL) {
            word_idf = idf_entry->score; }
          else {
            // Hardcoding anything missing as a term (corpus-wise)
            // that's what log2(8800 / 1) computes to anyway:
            word_idf = 13; }
          recorded_bin = true;
          bins[bin] += (word_tf * word_idf);
          tfc_normalization_divisor += (word_tf * word_idf) * (word_tf * word_idf);
          free(word_stem);
        }
      }
      free(words.range);
    }

    tfc_normalization_divisor = sqrt(tfc_normalization_divisor);

    free(sentences.range);
    free_DNM(paragraph_dnm);
    if (recorded_bin) {// Some paras have no content words, skip.
      // We have the paragraph vector, now map it into a TF/IDF vector and write down a labeled training instance:
      int label = is_definition(paragraph_node->parent) ? 1 : -1;
      fprintf(svm_input_file, "%d ",label);
      for (i=0; i<20000; i++) {
        if(bins[i] > 0.001) {
          //// We also need to normalize on the basis of paragraph length -- divide by total number of words in paragraph
          //fprintf(svm_input_file, "%d:%f ",i,bins[i]/paragraph_word_count);
          //normalize by using tfc_normalization_divisor instead
          fprintf(svm_input_file, "%d:%f", i, bins[i]/tfc_normalization_divisor);
        }}
      fprintf(svm_input_file,"\n");
    }
  }

  free_document_frequencies_hash(DF);
  free(paragraphs_result->nodesetval->nodeTab);
  free(paragraphs_result->nodesetval);
  xmlFree(paragraphs_result);
  xmlXPathFreeContext(xpath_context);
  xmlFreeDoc(document);
  return 0;
}


struct document_frequencies_hash* frequencies_hash_to_bins(struct score_hash *scores) {
  struct score_hash *s;
  struct document_frequencies_hash *tmp, *bins_hash = NULL;
  int bin_index = 0;
  HASH_SORT(scores, ascending_score_sort);
  for(s=scores; s != NULL; s = s->hh.next) {
    if (s->score >= 11.509115) { break; } // Filter >4 frequent words
    tmp = (struct document_frequencies_hash*)malloc(sizeof(struct document_frequencies_hash));
    tmp->word = strdup(s->word);
    tmp->count = ++bin_index;
    HASH_ADD_KEYPTR( hh, bins_hash, tmp->word, strlen(tmp->word), tmp );
  }
  fprintf(stderr, "Total bins: %d\n",bin_index);
  return bins_hash;
}


int main(int argc, char *argv[]) {
  char *source_directory, *destination_directory, *destination_filename;
  switch (argc) {
    case 0:
    case 1:
      fprintf(stderr, "Too few arguments: %d\nUsage: ./gen_libsvm_input source_dir/ [destination_dir/] [destination_file]\n",argc-1);
      exit(1);
    case 2:
      source_directory = argv[1];
      destination_directory = ".";
      destination_filename = "libsvm_input.txt";
      break;
    case 3:
      source_directory = argv[1];
      destination_directory = argv[2];
      destination_filename = "libsvm_input.txt";
      break;
    case 4:
      source_directory = argv[1];
      destination_directory = argv[2];
      destination_filename = argv[3];
      break;
    default:
      fprintf(stderr, "Too many arguments: %d\nUsage: ./gen_libsvm_input source_dir/ [destination_dir/] [destination_file]\n",argc-1);
      exit(1);
  }
  /* Load pre-computed corpus IDF scores */
  char idf_json_filename[FILENAME_BUFF_SIZE];
  sprintf(idf_json_filename, "%s/idf.json",destination_directory);
  json_object* idf_json = read_json(idf_json_filename);
  idf = json_to_score_hash(idf_json);
  json_object_put(idf_json);
  /* Map common words to bin positions */
  word_to_bin = frequencies_hash_to_bins(idf);

  char senna_opt_path[FILENAME_BUFF_SIZE];
  sprintf(senna_opt_path, "%s/third-party/senna/",LLAMAPUN_ROOT_PATH);

  /* Open the SVM input file for writing: */
  char svm_input_filename[FILENAME_BUFF_SIZE];
  sprintf(svm_input_filename, "%s/%s", destination_directory,destination_filename);
  
  svm_input_file = fopen(svm_input_filename,"w");

  //ftw(source_directory, process_file, 1);
  process_documents_in_directory(process_file, source_directory, LOADER_NORMALIZE_UNICODE, stderr);

  free_score_hash(idf);
  return 0;
}