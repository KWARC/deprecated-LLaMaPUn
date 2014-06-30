// Standard C libraries
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
// Hashes
#include <uthash.h>
// JSON
//#include <json-c/json.h>
#include "jsoninclude.h"
// XML DOM and XPath
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
// LLaMaPUn Utils
#include "llamapun_utils.h"
#include "stopwords.h"
#include "stemmer.h"
#include "llamapun_ngrams.h"
#include "unicode_normalizer.h"


struct stringcount {
  char *string;
  int counter;
  UT_hash_handle hh;
};

/*void add_ngram(const char *string, struct stringcount *sc) {  //does't work (macros??)
  //FILE *f = fopen("log.txt", "a");
  struct stringcount *tmp;
  HASH_FIND_STR(sc, string, tmp);
  if (tmp == NULL) {
    tmp = (struct stringcount *) malloc(sizeof(struct stringcount));
    tmp->string = strdup(string);
    tmp->counter = 1;
    HASH_ADD_KEYPTR(hh, sc, tmp->string, strlen(tmp->string), tmp);
  } else {    //ngram is already in hash
    tmp->counter++;
  }
}*/

json_object* llamapun_get_ngrams (xmlDocPtr doc) {
  char* log_message;
  if (doc == NULL) {
    fprintf(stderr, "Failed to parse workload!\n");
    log_message = "Fatal:LibXML:parse Failed to parse document.";
    return cortex_response_json("",log_message,-4); }
  xmlXPathContextPtr xpath_context = xmlXPathNewContext(doc);
  if(xpath_context == NULL) {
    fprintf(stderr,"Error: unable to create new XPath context\n");
    xmlFreeDoc(doc);
    log_message = "Fatal:LibXML:XPath unable to create new XPath context\n";
    return cortex_response_json("",log_message,-4); }

  /* Register XHTML, MathML namespaces */
  xmlXPathRegisterNs(xpath_context,  BAD_CAST "xhtml", BAD_CAST "http://www.w3.org/1999/xhtml");
  xmlXPathRegisterNs(xpath_context,  BAD_CAST "m", BAD_CAST "http://www.w3.org/1998/Math/MathML");
  xmlNsPtr xhtml_ns = xmlNewNs(xmlDocGetRootElement(doc),BAD_CAST "http://www.w3.org/1999/xhtml",BAD_CAST "xhtml");
  if (xhtml_ns == NULL) {
    perror("Failed to create XHTML namespace");  }

  xmlChar *sentence_xpath = (xmlChar*) "//xhtml:span[@class='ltx_sentence']";
  /* Generically stem all "math" elements to "[MathFormula]" words */
  xmlChar* math_xpath = (xmlChar*) "//m:math";
  xmlXPathObjectPtr xpath_math_result = xmlXPathEvalExpression(math_xpath,xpath_context);
  if (xpath_math_result != NULL) { // Nothing to do if there's no math in the document
    xmlNodeSetPtr math_nodeset = xpath_math_result->nodesetval;
    int math_index;
    for (math_index=0; math_index < math_nodeset->nodeNr; math_index++) {
      xmlNodePtr math_node = math_nodeset->nodeTab[math_index];
      xmlNodePtr new_math_word = xmlNewNode(xhtml_ns, BAD_CAST "span");
      xmlNewProp(new_math_word, BAD_CAST "class", BAD_CAST "ltx_word");
      xmlNodePtr math_stub_text = xmlNewText(BAD_CAST "[MathFormula]");
      xmlSetProp(new_math_word,BAD_CAST "id", BAD_CAST xmlGetProp(math_node,BAD_CAST "id"));
      xmlAddChild(new_math_word,math_stub_text);
      xmlReplaceNode(math_node,new_math_word);
    }
  }
  /* Stem all "a" anchor elements to their text content */
  xmlChar* anchor_xpath = (xmlChar*) "//xhtml:a";
  xmlXPathObjectPtr xpath_anchor_result = xmlXPathEvalExpression(anchor_xpath,xpath_context);
  if (xpath_anchor_result != NULL) { // Nothing to do if there's no math in the document
    xmlNodeSetPtr anchor_nodeset = xpath_anchor_result->nodesetval;
    int anchor_index;
    for (anchor_index=0; anchor_index < anchor_nodeset->nodeNr; anchor_index++) {
      xmlNodePtr anchor_node = anchor_nodeset->nodeTab[anchor_index];
      xmlNodePtr new_anchor_word = xmlNewNode(xhtml_ns, BAD_CAST "span");
      xmlNewProp(new_anchor_word, BAD_CAST "class", BAD_CAST "ltx_word");
      xmlNodePtr anchor_text = xmlNewText(BAD_CAST xmlNodeGetContent(anchor_node));
      xmlSetProp(new_anchor_word,BAD_CAST "id", BAD_CAST xmlGetProp(anchor_node,BAD_CAST "id"));
      xmlAddChild(new_anchor_word,anchor_text);
      xmlReplaceNode(anchor_node,new_anchor_word);
    }
  }

  //initialize everything for counting ngrams
  init_stemmer();
  struct stringcount *unigram_hash = NULL;
  struct stringcount * bigram_hash = NULL;
  struct stringcount *trigram_hash = NULL;
  if (access("../stopwords.json", R_OK)) {
    fprintf(stderr, "Error: Cannot open \"../stopwords.json\"\n");
    return NULL;
  }
  read_stopwords(json_object_from_file("../stopwords.json"));

  //Evaluate XPath
  xmlXPathObjectPtr xpath_sentence_result = xmlXPathEvalExpression(sentence_xpath, xpath_context);
  if(xpath_sentence_result == NULL) {
    fprintf(stderr,"Error: unable to evaluate sentence xpath expression \"%s\"\n", sentence_xpath);
    xmlXPathFreeContext(xpath_context);
    xmlFreeDoc(doc);
    log_message = "Fatal:LibXML:XPath unable to evaluate xpath expression\n";
    return cortex_response_json("",log_message,-4); }

  /* Sentence nodes: */
  xmlNodeSetPtr sentences_nodeset = xpath_sentence_result->nodesetval;
  /* Traverse each sentence and tag words */
  xmlChar *words_xpath = (xmlChar*) "//xhtml:span[@class='ltx_word']";
  int sentence_index;
  //loop over sentences
  for (sentence_index=0; sentence_index < sentences_nodeset->nodeNr; sentence_index++) {
    xmlNodePtr sentence = sentences_nodeset->nodeTab[sentence_index];
    xmlXPathContextPtr xpath_sentence_context = xmlXPathNewContext((xmlDocPtr)sentence);
    xmlXPathRegisterNs(xpath_sentence_context,  BAD_CAST "xhtml", BAD_CAST "http://www.w3.org/1999/xhtml");
    /* We want a list of words */
    xmlXPathObjectPtr words_xpath_result = xmlXPathEvalExpression(words_xpath, xpath_sentence_context);
    if(words_xpath_result == NULL) { // No words, skip
      xmlXPathFreeContext(xpath_sentence_context);
      continue; }
    xmlNodeSetPtr words_nodeset = words_xpath_result->nodesetval;
    char* word_input_string;
    size_t sentence_size;
    FILE *word_stream;
    word_stream = open_memstream (&word_input_string, &sentence_size);
    int word_count = words_nodeset->nodeNr;
    int words_index;
    int isnumber;
    size_t tmpindex;
    for (words_index=0; words_index < word_count; words_index++) {
      xmlNodePtr word_node = words_nodeset->nodeTab[words_index];
      char* word_content = (char*) xmlNodeGetContent(word_node);
      //add word_content to word_stream (replace numbers by "[number]")
      isnumber = 1;
      tmpindex = 0;
      while (tmpindex < strlen(word_content)) {
        if (!(isdigit(word_content[tmpindex]) || word_content[tmpindex] == '.')) {
          isnumber = 0;
          break;
        }
        tmpindex++;
      }
      if (isnumber) {
        fprintf(word_stream, "[number]");
      } else {
        fprintf(word_stream, "%s", word_content);
      }
      if (words_index < word_count - 1) fprintf(word_stream," ");   //no trailing space!!
    }

    //xmlElemDump(word_stream,doc,sentence);
    fclose(word_stream);

    //prepare for ngram extraction
    char *stemmed;
    char *tmp1,*tmp2,*tmp;
    normalize_unicode(word_input_string, &tmp);
    morpha_stem(tmp, &stemmed);
    free(tmp);
    char *index = stemmed;
    int foundStopword;
    int foundEnd = 0;
    int nonstopcounter;

/*
    FILE *f = fopen("log.txt", "a");
    //fprintf(f, " +++ NEW SENTENCE +++ ");
    fprintf(f, "%s", word_input_string);
    fclose(f);
*/

    //do the actual ngram extraction (iteratively)
    do {
      //step 1: remove leading stop words
      foundStopword = 1;
      while (foundStopword && !foundEnd) {
        tmp = index;
        while (!(isspace(*tmp) || !*tmp)) tmp++;
        foundEnd = (*tmp == '\0'); 
        *tmp = '\0';
        foundStopword = is_stopword(index);
        if (foundStopword) {
          index = tmp;   //move on
        }
        if (!foundEnd) *tmp = ' ';
        while (isspace(*index)) index++;  //go to first character of new word
      }
  
      //step 2: Count leading "non-stop" words
      nonstopcounter = 0;
      foundStopword = 0;
      if (foundEnd && !foundStopword) nonstopcounter = 1;
      tmp1 = index;
      while (!foundStopword && !foundEnd) {
        tmp2 = tmp1;    //tmp1 is supposed to point to the first letter of the first word that is not yet checked
                  //tmp2 shall point to the first whitespace thereafter
        while (!(isspace(*tmp2) || !*tmp2)) tmp2++;
        foundEnd = (*tmp2 == '\0'); 
        *tmp2 = '\0';
        foundStopword = is_stopword(tmp1);
        if (!foundStopword) {
          nonstopcounter++;
          tmp1 = tmp2;   //move on
        }
        if (!foundEnd) *tmp2 = ' ';
        while (isspace(*tmp1)) tmp1++;  //go to first character of new word
      }
      
      //step 3: process leading "non-stop" words
      tmp2 = index;   //tmp2 is starting position for next iteration
      struct stringcount *tmp_strcount;
      while (nonstopcounter--) {
        index = tmp2;
        tmp = index;
        //unigram
        while (!(isspace(*tmp) || !*tmp)) tmp++;
        *tmp = '\0';
        //add_ngram(index, unigram_hash);

        HASH_FIND_STR(unigram_hash, index, tmp_strcount);
        if (tmp_strcount == NULL) {
          tmp_strcount = (struct stringcount *) malloc(sizeof(struct stringcount));
          tmp_strcount->string = strdup(index);
          tmp_strcount->counter = 1;
          HASH_ADD_KEYPTR(hh, unigram_hash, tmp_strcount->string, strlen(tmp_strcount->string), tmp_strcount);
        } else {    //ngram is already in hash
          tmp_strcount->counter++;
        }

        *tmp = ' ';
        while (isspace(*++tmp));
        tmp2 = tmp;    //next index
        if (nonstopcounter < 1) continue;
        //bigram
        while (!(isspace(*tmp) || !*tmp)) tmp++;
        *tmp = '\0';
        //add_ngram(index, bigram_hash);
        
        HASH_FIND_STR(bigram_hash, index, tmp_strcount);
        if (tmp_strcount == NULL) {
          tmp_strcount = (struct stringcount *) malloc(sizeof(struct stringcount));
          tmp_strcount->string = strdup(index);
          tmp_strcount->counter = 1;
          HASH_ADD_KEYPTR(hh, bigram_hash, tmp_strcount->string, strlen(tmp_strcount->string), tmp_strcount);
        } else {    //ngram is already in hash
          tmp_strcount->counter++;
        }

        *tmp = ' ';
        while (isspace(*++tmp));
        if (nonstopcounter < 2) continue;
        //trigram
        while (!(isspace(*tmp) || !*tmp)) tmp++;
        *tmp = '\0';
        //add_ngram(index, trigram_hash);

        HASH_FIND_STR(trigram_hash, index, tmp_strcount);
        if (tmp_strcount == NULL) {
          tmp_strcount = (struct stringcount *) malloc(sizeof(struct stringcount));
          tmp_strcount->string = strdup(index);
          tmp_strcount->counter = 1;
          HASH_ADD_KEYPTR(hh, trigram_hash, tmp_strcount->string, strlen(tmp_strcount->string), tmp_strcount);
        } else {    //ngram is already in hash
          tmp_strcount->counter++;
        }

        *tmp = ' ';
        while (isspace(*++tmp));
      }
      index = tmp2;

  
    } while (!foundEnd);
    free(word_input_string);
    free(stemmed);
  }

  close_stemmer();
  free_stopwords();
  xmlXPathFreeContext(xpath_context);


  //now transfer data to json object
  json_object* response = json_object_new_object();
  json_object* unigrams = json_object_new_object();
  json_object*  bigrams = json_object_new_object();
  json_object* trigrams = json_object_new_object();

  struct stringcount *current;
  struct stringcount *tmp;

  HASH_ITER(hh, unigram_hash, current, tmp) {
    json_object_object_add(unigrams, current->string,
          json_object_new_int(current->counter));
    HASH_DEL(unigram_hash, current);
    free(current);
  }
  HASH_ITER(hh, bigram_hash, current, tmp) {
    json_object_object_add(bigrams, current->string,
          json_object_new_int(current->counter));
    HASH_DEL(bigram_hash, current);
    free(current);
  }
  HASH_ITER(hh, trigram_hash, current, tmp) {
    json_object_object_add(trigrams, current->string,
          json_object_new_int(current->counter));
    HASH_DEL(trigram_hash, current);
    free(current);
  }

  json_object_object_add(response, "unigrams", unigrams);
  json_object_object_add(response,  "bigrams",  bigrams);
  json_object_object_add(response, "trigrams", trigrams);
  return response;
}
