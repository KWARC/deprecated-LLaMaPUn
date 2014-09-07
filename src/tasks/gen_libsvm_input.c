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
struct score_hash* idf_scores;

/* Core traversal and analysis */
int file_counter=0;
xmlChar *paragraph_xpath = (xmlChar*) "//*[local-name()='section' and @class='ltx_section']//*[local-name()='div' and @class='ltx_para']";
xmlChar *relaxed_paragraph_xpath = (xmlChar*) "//*[local-name()='div' and @class='ltx_para']";

int process_file(const char *filename, const struct stat *status, int type) {
  if (type != FTW_F) return 0; //Not a file
  file_counter++;
  //if (file_counter > 100) { return 0; } // Limit for development
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
    /* Prepare a paragraph word vector: */
    struct document_frequencies_hash* paragraph_vector = NULL;
    /* 3. For every paragraph, tokenize sentences: */
    char* paragraph_text = paragraph_dnm->plaintext;
    dnmRanges sentences = tokenize_sentences(paragraph_text);
    /* 4. For every sentence, tokenize words */
    int sentence_index = 0;
    for (sentence_index = 0; sentence_index < sentences.length; sentence_index++) {
      // Obtaining only the content words here, disregard stopwords and punctuation
      dnmRanges words = tokenize_words(paragraph_text, sentences.range[sentence_index],
                                       TOKENIZER_ALPHA_ONLY | TOKENIZER_FILTER_STOPWORDS);
      int word_index;
      for(word_index=0; word_index<words.length; word_index++) {
        char* word_string = plain_range_to_string(paragraph_text, words.range[word_index]);
        char* word_stem;
        full_morpha_stem(word_string, &word_stem);
        free(word_string);
        // Note: SENNA's tokenization has some features to keep in mind:
        //  multi-symplectic --> "multi-" and "symplectic"
        //  Birkhoff's       --> "birkhoff" and "'s"

        // Add to the paragraph vector:
        record_word(&paragraph_vector, word_stem);
        free(word_stem);
      }
      free(words.range);
    }
    free(sentences.range);
    free_DNM(paragraph_dnm);
    // We have the paragraph vector, now map it into a TF/IDF vector and write down a labeled training instance:
    int label = is_definition(paragraph_node->parent);
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
  char *source_directory, *destination_directory;
  switch (argc) {
    case 0:
    case 1:
      source_directory = destination_directory = ".";
      break;
    case 2:
      source_directory = argv[1];
      destination_directory = ".";
      break;
    case 3:
      source_directory = argv[1];
      destination_directory = argv[2];
      break;
    default:
      fprintf(stderr, "Too many arguments: %d\nUsage: ./gen_TF_IDF source_dir/ destination_dir/\n",argc);
      exit(1);
  }

  /* Load pre-computed corpus IDF scores */
  char idf_json_filename[FILENAME_BUFF_SIZE];
  sprintf(idf_json_filename, "%s/idf.json",destination_directory);
  json_object* idf_json = read_json(idf_json_filename);
  struct score_hash *idf = json_to_score_hash(idf_json);

  json_object_put(idf_json);

  char senna_opt_path[FILENAME_BUFF_SIZE];
  sprintf(senna_opt_path, "%s/third-party/senna/",LLAMAPUN_ROOT_PATH);
  initialize_tokenizer(senna_opt_path);
  init_stemmer();

  //ftw(source_directory, process_file, 1);

  close_stemmer();
  free_tokenizer();
  xmlCleanupParser();
  free_score_hash(idf);
  return 0;
}
