#ifndef DOCUMENT_LOADER_H
#define DOCUMENT_LOADER_H

#include <libxml/parser.h>
#include <libxml/HTMLparser.h>


#define LOADER_NORMALIZE_UNICODE 1 << 0
#define LOADER_CHECK_LANGUAGE 1 << 1

#define WORDS_NORMALIZE_NUMBERS 1 << 0
#define WORDS_STEM_WORDS 1 << 1

void init_document_loader();
void close_document_loader();

//note: if return value of function is non-zero, traversal will be interrupted
void traverse_docs_in_dir(char *dir, int (*function)(xmlDocPtr, const char *), long parameters, FILE *logfile);

//calls function for each node, to which xpath applies.
//The first argument of the function call is an array of words, the second one the number of words in that node
//parameters can be WORDS_NORMALIZE_NUMBERS or WORDS_STEM_WORDS
//note: the memory for the word array will be cleaned up by this function, you don't need to take care of it
void get_words_of_xpath(xmlDocPtr document, const char * xpath, void (*function)(char *[], size_t), long parameters, FILE *logfile, long dnm_parameters);

#endif