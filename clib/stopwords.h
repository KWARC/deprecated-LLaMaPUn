#ifndef STOPWORDS_H_INCLUDED
#define STOPWORDS_H_INCLUDED

#include <json.h>

void read_stopwords(json_object *);
void free_stopwords();
int is_stopword(const char *);

#endif