#define interactive   /*required for morpha to work properly*/
#include <ctype.h>
#include "third-party/morpha/morpha.yy.c"
#include "normalizer.h"
#include <stdio.h>

void init_normalizer() {
	/* Initialize options */
	UnSetOption(print_affixes);
	SetOption(change_case);
	UnSetOption(tag_output);
	UnSetOption(fspec);

	//Initialize in and out stream
	size_t insize, outsize;   //we're not interested in the size
	morpha_instream = open_memstream(&morpha_instream_buff_ptr, &insize);
	morpha_outstream = open_memstream(&morpha_outstream_buff_ptr, &outsize);

	yyout = morpha_outstream;
	yyin = morpha_instream;

	state = any;

	read_verbstem("third-party/morpha/verbstem.list");
}

void normalize(const char *sentence, char **normalized) {
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
	*normalized = morpha_outstream_buff_ptr;

	size_t outsize;
	morpha_outstream = open_memstream(&morpha_outstream_buff_ptr, &outsize);
}

void close_normalizer() {
	fclose(morpha_instream);
	fclose(morpha_outstream);
	free(morpha_instream_buff_ptr);
	free(morpha_outstream_buff_ptr);
}
