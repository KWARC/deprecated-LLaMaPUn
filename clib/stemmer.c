#define interactive   /*required for morpha to work properly*/
#include <ctype.h>
#include "third-party/morpha/morpha.yy.c"
#include "stemmer.h"
#include <stdio.h>

void init_stemmer() {
	/* Initialize options */
	UnSetOption(print_affixes);
	SetOption(change_case);
	UnSetOption(tag_output);
	UnSetOption(fspec);

	//Initialize in and out stream
	
	state = any;

	read_verbstem("third-party/morpha/verbstem.list");
}

void morpha_stem(const char *sentence, char **stemmed) {
	size_t insize, outsize;   //we're not interested in the size
	morpha_instream = open_memstream(&morpha_instream_buff_ptr, &insize);
	morpha_outstream = open_memstream(&morpha_outstream_buff_ptr, &outsize);

	yyout = morpha_outstream;
	yyin = morpha_instream;

	fprintf(morpha_instream, "%s", sentence);

	int c;
	while ((c = getc(morpha_instream)) != EOF) {
		ungetc(c, morpha_instream);
		BEGIN(state);
		yylex();
	}
		
	BEGIN(state);
	yylex();

	fclose(morpha_outstream);
	*stemmed = morpha_outstream_buff_ptr;

	//size_t outsize;
	//morpha_outstream = open_memstream(&morpha_outstream_buff_ptr, &outsize);
	fclose(morpha_instream);
	free(morpha_instream_buff_ptr);
}

void close_stemmer() {
	//fclose(morpha_instream);
	//free(morpha_instream_buff_ptr);
	//fclose(morpha_outstream);
	//free(morpha_outstream_buff_ptr);
}
