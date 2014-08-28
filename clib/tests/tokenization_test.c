#include <stdio.h>
#include <stdlib.h>

#include "../llamapun_tokenizer.h"
#include <libxml/parser.h>
#include <libxml/tree.h>

int main(void) {
  //load example document
  fprintf(stderr, "\n\n Tokenization tests\n\n");
  xmlDocPtr mydoc = xmlReadFile("../../t/documents/1311.0066.xhtml", NULL, XML_PARSE_RECOVER | XML_PARSE_NONET);
  if (mydoc == NULL) { return 1;}
  //Create DNM, with normalized math tags, and ignoring cite tags
  dnmPtr mydnm = createDNM(mydoc, DNM_NORMALIZE_TAGS);
  if (mydnm == NULL) { return 1;}
  char* text = mydnm->plaintext;
  dnmRanges sentences = tokenize_sentences(text);
  if (sentences.length < 700) {   //something could have gone wrong...
    fprintf(stderr, "There were too few sentences tokenized!\n");
    return 1;
  }
  if (sentences.length > 800) {   //something could have gone wrong...
    fprintf(stderr, "There were too many sentences tokenized!\n");
    return 1;
  }

  initialize_tokenizer();
  int sentence_index;
  for (sentence_index=0; sentence_index < sentences.length; sentence_index++) {
    dnmRanges words = tokenize_words(sentences.range[sentence_index],text);
    fprintf(stderr,"=====\n");
    fprintf(stderr,"%.*s\n",(sentences.range[sentence_index].end-sentences.range[sentence_index].start+1),(text+sentences.range[sentence_index].start));
    fprintf(stderr,"-----\n");
    display_ranges(words, mydnm->plaintext);
  }

  //clean up
  free_tokenizer();
  free(sentences.range);
  freeDNM(mydnm);
  xmlFreeDoc(mydoc);
  xmlCleanupParser();

  return 0;
}
