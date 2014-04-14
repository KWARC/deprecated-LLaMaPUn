// Standard C libraries
#include <stdio.h>
#include <string.h>
#include <ctype.h>

// Senna POS tagger
#include <senna/SENNA_utils.h>
#include <senna/SENNA_Hash.h>
#include <senna/SENNA_Tokenizer.h>
#include <senna/SENNA_POS.h>
// Hashes
#include <uthash.h>
// JSON
#include <json-c/json.h>
// XML DOM and XPath
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
// LLaMaPUn Utils
#include "llamapun_utils.h"

json_object* dom_to_pos_annotations (xmlDocPtr doc) {
  char* log_message;
  if (doc == NULL) {
    fprintf(stderr, "Failed to parse workload!\n");
    log_message = "Fatal:LibXML:parse Failed to parse document.";
    return cortex_response_json("",log_message,-4); }
  xmlXPathContextPtr xpath_context;
  xmlXPathObjectPtr xpath_result;
  xpath_context = xmlXPathNewContext(doc);
  if(xpath_context == NULL) {
    fprintf(stderr,"Error: unable to create new XPath context\n");
    xmlFreeDoc(doc);
    log_message = "Fatal:LibXML:XPath unable to create new XPath context\n";
    return cortex_response_json("",log_message,-4); }

  /* Register XHTML namespace */
  xmlXPathRegisterNs(xpath_context,  BAD_CAST "xhtml", BAD_CAST "http://www.w3.org/1999/xhtml");
  xmlChar *xpath = (xmlChar*) "//xhtml:span[@class='ltx_sentence']";

  /* Find sentences: */
  xpath_result = xmlXPathEvalExpression(xpath, xpath_context);
  if(xpath_result == NULL) {
    fprintf(stderr,"Error: unable to evaluate xpath expression \"%s\"\n", xpath);
    xmlXPathFreeContext(xpath_context);
    xmlFreeDoc(doc);
    log_message = "Fatal:LibXML:XPath unable to evaluate xpath expression\n";
    return cortex_response_json("",log_message,-4); }

  /* Sentence nodes: */
  xmlNodeSetPtr sentences_nodeset = xpath_result->nodesetval;
  /* Traverse each sentence and tag words */
  xmlChar *words_xpath = (xmlChar*) "//xhtml:span[@class='ltx_word']";
  int sentence_index;
  for (sentence_index=0; sentence_index < sentences_nodeset->nodeNr; sentence_index++) {
    xmlNodePtr sentence = sentences_nodeset->nodeTab[sentence_index];
    xmlXPathContextPtr xpath_sentence_context = xmlXPathNewContext((xmlDocPtr)sentence);
    xmlXPathRegisterNs(xpath_sentence_context,  BAD_CAST "xhtml", BAD_CAST "http://www.w3.org/1999/xhtml");
    /* We want a list of words and a corresponding list of IDs */
    xmlXPathObjectPtr words_xpath_result = xmlXPathEvalExpression(words_xpath, xpath_sentence_context);
    if(words_xpath_result == NULL) { // No words, skip
      xmlXPathFreeContext(xpath_sentence_context);
      continue; }
    xmlNodeSetPtr words_nodeset = words_xpath_result->nodesetval;
//    const xmlChar **words;
//    xmlChar **ids;
//    const xmlChar **words_pointer = words;
//    xmlChar **ids_pointer = ids;
//    int words_index;
//    for (words_index=0; words_index < words_nodeset->nodeNr; words_index++) {
//      xmlNodePtr word_node = words_nodeset->nodeTab[words_index];
//      *words_pointer++ = xmlNodeGetContent(word_node);
//      *ids_pointer++ = xmlGetProp(word_node,"id");
//    }
    // Obtain POS tags:
    char* opt_path = NULL;
    int *pos_labels = NULL;
    //SENNA_POS *pos = SENNA_POS_new(opt_path, "third-party/senna/data/pos.dat");
    //pos_labels = SENNA_POS_forward(pos, tokens->word_idx, tokens->caps_idx, tokens->suff_idx, tokens->n);

    /* We obtain the corresponding list of POS tags for the list of words */
    /* We return a JSON object that has "ID => POS tag" as result structure.*/
  }

  // Freedom
  xmlXPathFreeContext(xpath_context);
  return NULL; // TODO Return a proper json object
}
