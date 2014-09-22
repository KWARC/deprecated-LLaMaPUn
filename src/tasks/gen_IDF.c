#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ftw.h>
#include <uthash.h>
#include <math.h>
#include <sys/stat.h>
#include <llamapun/json_include.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include <llamapun/local_paths.h>
#include <llamapun/utils.h>
#include <llamapun/dnmlib.h>
#include <llamapun/unicode_normalizer.h>
#include <llamapun/word_normalizer.h>
#include <llamapun/tokenizer.h>
#include <llamapun/stemmer.h>

#define FILENAME_BUFF_SIZE 2048

// Global frequencies hash:
struct document_frequencies_hash* CF;

/* Core traversal and analysis */
unsigned long file_counter=0;
unsigned long corpus_size=0;
xmlChar *paragraph_xpath = (xmlChar*) "//*[local-name()='section' and @class='ltx_section']//*[local-name()='div' and @class='ltx_para']";
xmlChar *relaxed_paragraph_xpath = (xmlChar*) "//*[local-name()='div' and @class='ltx_para']";

int process_file(const char *filename, const struct stat *status, int type) {
  UNUSED(status);
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

  struct document_frequencies_hash* DF = NULL;
  /* Iterate over each paragraph: */
  for (para_index=0; para_index < paragraph_nodeset->nodeNr; para_index++) {
    xmlNodePtr paragraph_node = paragraph_nodeset->nodeTab[para_index];
    // Obtain NLP-friendly plain-text of the paragraph:
    // -- We want to skip tags, as we only are interested in word counts for terms in TF-IDF
    dnmPtr paragraph_dnm = create_DNM(paragraph_node, DNM_NORMALIZE_TAGS | DNM_IGNORE_LATEX_NOTES);
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
      // Obtaining only the content words here, disregard stopwords and punctuation
      dnmRanges words = tokenize_words(paragraph_text, sentences.range[sentence_index],TOKENIZER_ACCEPT_ALL);
      // N  ote: SENNA's tokenization has some features to keep in mind:
      //  multi-symplectic --> "multi-" and "symplectic"
      //  Birkhoff's       --> "birkhoff" and "'s"
      int word_index;
      for(word_index=0; word_index<words.length; word_index++) {
        char* word_string = plain_range_to_string(paragraph_text, words.range[word_index]);
        //fprintf(stderr,"%s -> ",word_string);
        char* word_stem;
        /* Ensure stemming is an invariant (tilings -> tiling -> tile -> tile) */
        full_morpha_stem(word_string, &word_stem);
        free(word_string);
        /* Normalize word (numbers go to NNUMBER, so do numeric labels) */
        normalize_word(&word_stem);
        //fprintf(stderr,"%s\n",word_stem);
        // Add to the document frequency
        record_word(&DF, word_stem);
        free(word_stem);
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
  xmlFreeDoc(doc);

  /* The document has content, record DF keys in CF */
  // For IDF we are recording _IF_ a term occurred in a document, NOT how often
  corpus_size++;
  struct document_frequencies_hash *w;
  for(w=DF; w != NULL; w = w->hh.next) {
    record_word(&CF,w->word);
  }
  free_document_frequencies_hash(DF);
  fprintf(stderr,"Completed document #%lu\n",file_counter);
  return 0;
}

struct score_hash* compute_idf(unsigned long corpus_size, struct document_frequencies_hash* corpus_frequencies) {
 struct score_hash* idf_scores = NULL;
 struct document_frequencies_hash *entry;
 for(entry=corpus_frequencies; entry != NULL; entry = entry->hh.next) {
    double score = 1 + log2(corpus_size / (1.0 + entry->count));
    struct score_hash *this_score = (struct score_hash*) malloc(sizeof(struct score_hash));
    this_score->word = strdup(entry->word);
    this_score->score = score;
    HASH_ADD_KEYPTR( hh, idf_scores, this_score->word, strlen(this_score->word), this_score );
 }
 return idf_scores;
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

  char senna_opt_path[FILENAME_BUFF_SIZE];
  sprintf(senna_opt_path, "%s/third-party/senna/",LLAMAPUN_ROOT_PATH);
  initialize_tokenizer(senna_opt_path);
  init_stemmer();
  initialize_word_normalizer();

  corpus_size = 0;
  ftw(source_directory, process_file, 1);
  fprintf(stderr,"Analyzed corpus of %lu documents -- complete.\n",corpus_size);

  close_word_normalizer();
  close_stemmer();
  free_tokenizer();
  xmlCleanupParser();

  char frequency_filename[FILENAME_BUFF_SIZE];
  char idf_filename[FILENAME_BUFF_SIZE];
  sprintf(frequency_filename, "%s/corpus_frequencies.json", destination_directory);
  sprintf(idf_filename, "%s/idf.json", destination_directory);

  // Record the corpus size:
  struct document_frequencies_hash* csize_entry = (struct document_frequencies_hash*)malloc(sizeof(struct document_frequencies_hash));
  csize_entry->word = "__corpus_size";
  csize_entry->count = corpus_size;
  HASH_ADD_KEYPTR( hh, CF, csize_entry->word, strlen(csize_entry->word), csize_entry );

  /* What have we collected? Write frequencies to file: */
  HASH_SORT(CF, descending_count_sort);
  json_object* CF_json = document_frequencies_hash_to_json(CF);
  #ifndef JSON_OBJECT_OBJECT_GET_EX_DEFINED
  json_object_to_file(frequency_filename, CF_json);
  #else
  json_object_to_file_ext(frequency_filename, CF_json, JSON_C_TO_STRING_PRETTY);
  #endif
  json_object_put(CF_json);

  // Remove corpus size, we only needed it for bookkeeping
  HASH_DEL(CF, csize_entry);
  free(csize_entry);

  struct score_hash* idf_scores = compute_idf(corpus_size, CF);
  free_document_frequencies_hash(CF);

  HASH_SORT(idf_scores, descending_score_sort);
  json_object* idf_scores_json = score_hash_to_json(idf_scores);
  free_score_hash(idf_scores);

  #ifndef JSON_OBJECT_OBJECT_GET_EX_DEFINED
  json_object_to_file(idf_filename, idf_scores_json);
  #else
  json_object_to_file_ext(idf_filename, idf_scores_json,JSON_C_TO_STRING_PRETTY);
  #endif
  json_object_put(idf_scores_json);

  return 0;
}
