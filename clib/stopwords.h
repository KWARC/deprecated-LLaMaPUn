#ifndef STOPWORDS_H_INCLUDED
#define STOPWORDS_H_INCLUDED

//#include <json-c/json.h>
#include "jsoninclude.h"

void read_stopwords_from_json(json_object *);
void load_stopwords();   //loads a set of math stopwords
void free_stopwords();
int is_stopword(const char *);

#endif