#ifndef DOCUMENT_LOADER_H
#define DOCUMENT_LOADER_H

#include <libxml/parser.h>
#include <libxml/HTMLparser.h>
#include "llamapun/dnmlib.h"


#define LOADER_NORMALIZE_UNICODE 1 << 0
#define LOADER_CHECK_LANGUAGE 1 << 1

//not stemming, but numbers
#define WORDS_NORMALIZE_WORDS 1 << 0
#define WORDS_STEM_WORDS 1 << 1
#define WORDS_MARK_END_OF_SENTENCE 1 << 2

void init_document_loader();
void close_document_loader();

//note: if return value of function is non-zero, traversal will be interrupted
void process_documents_in_directory(int (*function)(xmlDocPtr, const char *), char *dir, long parameters, FILE *logfile);


//calls function for each node, to which xpath applies.
//The first argument of the function call is an array of words, the second one the number of words in that node
//parameters can be WORDS_NORMALIZE_NUMBERS or WORDS_STEM_WORDS
//note: the memory for the word array will be cleaned up by this function, you don't need to take care of it
//returns 1, if nodes were found, otherwise 0
int with_words_at_xpath(void (*function)(char *[], size_t, xmlNodePtr), xmlDocPtr document, const char * xpath, FILE *logfile, long parameters, long dnm_parameters);


#endif
