/*! \defgroup stemmer A Library for Stemming Words
    Basically, this library is a small wrapper around
	the (therefore slightly modified) morpha stemmer
	from the University of Sussex.
	@file
*/

#ifndef NORMALIZER_H_INCLUDED
#define NORMALIZER_H_INCLUDED
#include <stdio.h>

//used as an interface to morpha:
//(I've redirected the reading and writing from stdin/stdout to these streams)
FILE *morpha_instream;
FILE *morpha_outstream;
char *morpha_instream_buff_ptr;
char *morpha_outstream_buff_ptr;

/*! Initializes the stemmer */
void init_stemmer();

/*! closes the stemmer */
void close_stemmer();

/*! stems a sentence
    @param input the input string
	@param output pointer to the stemmed output
*/
void morpha_stem(const char *input, char **output);

#endif
