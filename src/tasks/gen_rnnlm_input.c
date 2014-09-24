#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ftw.h>
#include <uthash.h>
#include <math.h>
#include <sys/stat.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include <llamapun/local_paths.h>
#include <llamapun/json_include.h>
#include <llamapun/utils.h>
#include <llamapun/dnmlib.h>
#include <llamapun/unicode_normalizer.h>
#include <llamapun/tokenizer.h>
#include <llamapun/stemmer.h>
#include <llamapun/crf.h>
#include <llamapun/word_normalizer.h>

#define FILENAME_BUFF_SIZE 2048

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
struct document_frequencies_hash* corpus_frequencies;
// File handle for the svm input
FILE *rnn_positive_file, *rnn_negative_file;

/* Core traversal and analysis */
int file_counter=0;
xmlChar *paragraph_xpath = (xmlChar*)
    "//*[local-name()='section' and @class='ltx_section']//*[local-name()='div' and @class='ltx_para']";
xmlChar *relaxed_paragraph_xpath = (xmlChar*) "//*[local-name()='div' and @class='ltx_para']";

int process_file(const char *filename, const struct stat *status, int type) {
  if (type != FTW_F) return 0; //Not a file
  UNUSED(status);
  file_counter++;
  //if (file_counter > 2) { return 0; } // Limit for development
  fprintf(stderr, " Loading %s\n",filename);
  xmlDoc *doc = read_document(filename);
  if (doc == NULL) return 0;   //error message printed by read_document
  /* 1. Normalize Unicode */
  unicode_normalize_dom(doc);
  /* 2. Select paragraphs */
  xmlXPathContextPtr xpath_context = xmlXPathNewContext(doc);
  if(xpath_context == NULL) {
    fprintf(stderr,"Error: unable to create new XPath context for %s\n",filename);
    xmlFreeDoc(doc);
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
      xmlFreeDoc(doc);
      return 0; } }
  xmlNodeSetPtr paragraph_nodeset = paragraphs_result->nodesetval;
  int para_index;

  /* ------------------------------------------------- */
  /* ------------------------------------------------- */
  /* Emit RNNLM input for paragraph:                   */
  for (para_index=0; para_index < paragraph_nodeset->nodeNr; para_index++) {
    xmlNodePtr paragraph_node = paragraph_nodeset->nodeTab[para_index];
    bool definition_paragraph = is_definition(paragraph_node->parent);
    // Obtain NLP-friendly plain-text of the paragraph:
    // -- We want to skip tags, as we only are interested in word counts for terms in TF-IDF
    dnmPtr paragraph_dnm = create_DNM(paragraph_node, DNM_NORMALIZE_TAGS | DNM_IGNORE_LATEX_NOTES);
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
    for (sentence_index = 0; sentence_index < sentences.length; sentence_index++) {
      // Obtaining only the content words here, disregard stopwords and punctuation
      dnmRanges words = tokenize_words(paragraph_text, sentences.range[sentence_index],
                                       TOKENIZER_ACCEPT_ALL);
      int word_index;
      for(word_index=0; word_index<words.length; word_index++) {

        char* word_string = plain_range_to_string(paragraph_text, words.range[word_index]);
        normalize_word(&word_string);

        char* word_stem;
        full_morpha_stem(word_string, &word_stem);
        free(word_string);

        // Record word (if common), or "<unk>" if rare
        paragraph_word_count++;
        struct document_frequencies_hash *frequency_entry;
        HASH_FIND_STR(corpus_frequencies, word_stem, frequency_entry);
        if ((frequency_entry != NULL) && (frequency_entry->count > 4)) {
          // Not a rare word -> print as-is.
          if (definition_paragraph) {
            fprintf(rnn_positive_file, "%s ", word_stem); }
          else {
            fprintf(rnn_negative_file, "%s ", word_stem); }
        } else {
          // Rare word -> print "unk"
          if (definition_paragraph) {
            fprintf(rnn_positive_file, "<unk> "); }
          else {
            fprintf(rnn_negative_file, "<unk> "); }
        }
        free(word_stem);
      }
      free(words.range);
    }
    free(sentences.range);
    free_DNM(paragraph_dnm);
    if (paragraph_word_count > 0) {
      if (definition_paragraph) {
        fprintf(rnn_positive_file, "\n"); }
      else {
        fprintf(rnn_negative_file, "\n"); }
    }
  }

  free(paragraphs_result->nodesetval->nodeTab);
  free(paragraphs_result->nodesetval);
  xmlFree(paragraphs_result);
  xmlXPathFreeContext(xpath_context);
  xmlFreeDoc(doc);

  fprintf(stderr,"Completed document #%d\n",file_counter);
  return 0;
}

int main(int argc, char *argv[]) {
  char *source_directory, *destination_directory, *destination_filename;
  switch (argc) {
    case 0:
    case 1:
      fprintf(stderr, "Too few arguments: %d\nUsage: ./gen_rnnlm_input source_dir/ [destination_dir/] [destination_file]\n",argc-1);
      exit(1);
    case 2:
      source_directory = argv[1];
      destination_directory = ".";
      destination_filename = "rnnlm.input";
      break;
    case 3:
      source_directory = argv[1];
      destination_directory = argv[2];
      destination_filename = "rnnlm.input";
      break;
    case 4:
      source_directory = argv[1];
      destination_directory = argv[2];
      destination_filename = argv[3];
      break;
    default:
      fprintf(stderr, "Too many arguments: %d\nUsage: ./gen_rnnlm_input source_dir/ [destination_dir/] [destination_file]\n",argc-1);
      exit(1);
  }


  char senna_opt_path[FILENAME_BUFF_SIZE];
  sprintf(senna_opt_path, "%s/third-party/senna/",LLAMAPUN_ROOT_PATH);
  initialize_tokenizer(senna_opt_path);
  init_stemmer();
  initialize_word_normalizer();
  /* Load the corpus frequencies: */
  char frequencies_json_filename[FILENAME_BUFF_SIZE];
  sprintf(frequencies_json_filename, "%s/corpus_frequencies.json",destination_directory);
  json_object* corpus_frequencies_json = read_json(frequencies_json_filename);
  corpus_frequencies = json_to_document_frequencies_hash(corpus_frequencies_json);
  json_object_put(corpus_frequencies_json);

  /* Open two RNN input file for writing: */
  char rnn_positive_filename[FILENAME_BUFF_SIZE], rnn_negative_filename[FILENAME_BUFF_SIZE];
  sprintf(rnn_positive_filename, "%s/%s.positive", destination_directory,destination_filename);
  sprintf(rnn_negative_filename, "%s/%s.negative", destination_directory,destination_filename);
  rnn_positive_file = fopen(rnn_positive_filename,"w");
  rnn_negative_file = fopen(rnn_negative_filename,"w");

  ftw(source_directory, process_file, 1);

  fclose(rnn_positive_file);
  fclose(rnn_negative_file);

  close_stemmer();
  close_word_normalizer();
  free_tokenizer();
  xmlCleanupParser();
  free_document_frequencies_hash(corpus_frequencies);
  return 0;
}
