/*! \defgroup stopwords Stopword Library
   A small library which provides a function to check
   whether a word is a regarded a stopword.
   @file
*/

#ifndef STOPWORDS_H_INCLUDED
#define STOPWORDS_H_INCLUDED
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <uthash.h>
#include <llamapun/json_include.h>
#include <llamapun/stopwords.h>

/*! loads the stopwords from a JSON array */
void read_stopwords_from_json(json_object *);

/*! loads the stopwords from a predefined set of math specialized stopwords */
void load_stopwords();

/*! frees the currently loaded set of stopwords */
void free_stopwords();

/*! Checks whether a word is regarded a stopword.
    Note that this function is case sensitive.
    @param word the word to be checked
    @retval returns 1 if the word is a stopword, otherwise 0
*/
int is_stopword(const char * word);

#endif
