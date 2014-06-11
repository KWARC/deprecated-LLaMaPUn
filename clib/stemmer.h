#ifndef NORMALIZER_H_INCLUDED
#define NORMALIZER_H_INCLUDED
#include <stdio.h>

FILE *morpha_instream;
FILE *morpha_outstream;
char *morpha_instream_buff_ptr;
char *morpha_outstream_buff_ptr;

void init_stemmer();
void close_stemmer();
void morpha_stem(const char *input, char **output);

#endif
