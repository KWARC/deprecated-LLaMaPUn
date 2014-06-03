#include "morpha/morpha.yy.c"
#include "normalizer.h"
#include <stdio.h>

void init_normalizer() {
	/* Initialize options */
	UnSetOption(print_affixes);
	SetOption(change_case);
	UnSetOption(tag_output);
	UnSetOption(fspec);

	//Initialize in and out stream
	size_t insize, outsize;
	char *instrptr;
	char *outstrptr;
	morpha_instream = open_memstream(&instrptr, &insize);
	morpha_outstream = open_memstream(&outstrptr, &outsize);

	yyout = morpha_outstream;
	yyin = morpha_instream;

	state = any;

	read_verbstem("morpha/verbstem.list");
}

void normalize(char *word, char *normalized, size_t size) {
	fprintf(morpha_instream, "%s", word);

	int c;
	while ((c = getc(morpha_instream)) != EOF) {
		ungetc(c, morpha_instream);
		BEGIN(state);
		yylex();
	}

	fgets(normalized, size, morpha_outstream);
}

void close_normalizer() {
	fclose(morpha_instream);
	fclose(morpha_outstream);
}
