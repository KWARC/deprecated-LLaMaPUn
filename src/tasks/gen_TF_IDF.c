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
struct corpus_frequencies_hash* CF;

/* Core traversal and analysis */
int file_counter=0;
xmlChar *paragraph_xpath = (xmlChar*)
    "//*[local-name()='section' and @class='ltx_section']//*[local-name()='div' and @class='ltx_para']";
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
    dnmPtr paragraph_dnm = create_DNM(paragraph_node, DNM_SKIP_TAGS | DNM_IGNORE_LATEX_NOTES);
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
      dnmRanges words = tokenize_words(paragraph_text, sentences.range[sentence_index],
                                       TOKENIZER_ALPHA_ONLY | TOKENIZER_FILTER_STOPWORDS);
      int word_index;
      for(word_index=0; word_index<words.length; word_index++) {
        char* word_string = plain_range_to_string(paragraph_text, words.range[word_index]);
        normalize_word(&word_string);
        char* word_stem;
        full_morpha_stem(word_string, &word_stem);
        /* Ensure stemming is an invariant (tilings -> tiling -> tile -> tile) */
        free(word_string);
        // Note: SENNA's tokenization has some features to keep in mind:
        //  multi-symplectic --> "multi-" and "symplectic"
        //  Birkhoff's       --> "birkhoff" and "'s"
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

  /* The document has content, make an entry in Corpus Frequencies hash: */
  HASH_SORT(DF, descending_count_sort);
  struct corpus_frequencies_hash* doc_entry = (struct corpus_frequencies_hash*)malloc(sizeof(struct corpus_frequencies_hash));
  doc_entry->document = strdup(filename);
  doc_entry->F = DF;
  HASH_ADD_KEYPTR( hh, CF, doc_entry->document, strlen(doc_entry->document), doc_entry );

  fprintf(stderr,"Completed document #%d\n",file_counter);
  return 0;
}

struct corpus_scores_hash* compute_tf_idf(struct corpus_frequencies_hash* corpus_frequencies) {
  /* Compute IDF hash: */
  struct corpus_scores_hash *corpus_scores = NULL;
  struct document_frequencies_hash *idf = NULL;
  /* We make one pass over the corpus frequencies and compute TF and IDF at the same time */
  struct corpus_frequencies_hash *d;
  int document_count = 0; // Number of documents in the corpus
  /* Compute global variables first: */
  for(d=corpus_frequencies; d != NULL; d = d->hh.next) {
    struct document_frequencies_hash *w;
    document_count++;
    for(w=d->F; w != NULL; w = w->hh.next) {
      record_word(&idf,w->word);
    }
  }
  for(d=corpus_frequencies; d != NULL; d = d->hh.next) {
    struct document_frequencies_hash *w;
    int doc_max_count = 0;
    // dpc_max_count = Max word frequency in document
    for(w=d->F; w != NULL; w = w->hh.next) {
      if (doc_max_count < w->count) {
        doc_max_count = w->count;
      }
    }
    // Now that we have the doc_max_count, compute the TFxIDF scores:
    struct score_hash *doc_tf_idf = NULL;
    for(w=d->F; w != NULL; w = w->hh.next) {
      double w_tf = 0.5 + (0.5 * w->count)/doc_max_count;
      struct document_frequencies_hash* w_idf_hash;
      HASH_FIND_STR(idf, w->word, w_idf_hash);
      double w_idf = log2(document_count / (1.0+w_idf_hash->count));
      struct score_hash *this_score = (struct score_hash*) malloc(sizeof(struct score_hash));
      this_score->word = strdup(w->word);
      this_score->score = w_tf * w_idf;
      HASH_ADD_KEYPTR( hh, doc_tf_idf, this_score->word, strlen(this_score->word), this_score );
    }
    HASH_SORT(doc_tf_idf, descending_score_sort);
    struct corpus_scores_hash *doc_scores = (struct corpus_scores_hash*) malloc(sizeof(struct corpus_scores_hash));
    doc_scores->document = strdup(d->document);
    doc_scores->scores = doc_tf_idf;
    HASH_ADD_KEYPTR( hh, corpus_scores, doc_scores->document, strlen(doc_scores->document), doc_scores );
  }
  free_document_frequencies_hash(idf);

  return corpus_scores;
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

  ftw(source_directory, process_file, 1);

  close_stemmer();
  free_tokenizer();
  xmlCleanupParser();

  /* What have we collected? Write frequencies to file: */
  HASH_SORT(CF, document_key_sort);
  json_object* CF_json = corpus_frequencies_hash_to_json(CF);

  char frequency_filename[FILENAME_BUFF_SIZE];
  char tf_idf_filename[FILENAME_BUFF_SIZE];
  sprintf(frequency_filename, "%s/corpus_frequencies.json", destination_directory);
  sprintf(tf_idf_filename, "%s/tf_idf.json", destination_directory);

  #ifndef JSON_OBJECT_OBJECT_GET_EX_DEFINED
  json_object_to_file(frequency_filename, CF_json);
  #else
  json_object_to_file_ext(frequency_filename, CF_json, JSON_C_TO_STRING_PRETTY);
  #endif
  json_object_put(CF_json);

  struct corpus_scores_hash* corpus_scores = compute_tf_idf(CF);
  json_object* corpus_scores_json = corpus_scores_hash_to_json(corpus_scores);
  #ifndef JSON_OBJECT_OBJECT_GET_EX_DEFINED
  json_object_to_file(tf_idf_filename, corpus_scores_json);
  #else
  json_object_to_file_ext(tf_idf_filename, corpus_scores_json,JSON_C_TO_STRING_PRETTY);
  #endif
  json_object_put(corpus_scores_json);
  free_corpus_scores_hash(corpus_scores);
  free_corpus_frequencies_hash(CF);

  return 0;
}
