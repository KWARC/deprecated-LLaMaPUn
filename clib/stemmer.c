#define interactive   /*required for morpha to work properly*/
#include <ctype.h>
#include "third-party/morpha/morpha.yy.c"
#include "stemmer.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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

//from http://stackoverflow.com/questions/779875/what-is-the-function-to-replace-string-in-c
// You must free the result if result is non-NULL.
char *str_replace(char *orig, char *rep, char *with) {
    char *result; // the return string
    char *ins;    // the next insert point
    char *tmp;    // varies
    int len_rep;  // length of rep
    int len_with; // length of with
    int len_front; // distance between rep and end of last rep
    int count;    // number of replacements

    if (!orig)
        return NULL;
    if (!rep)
        rep = "";
    len_rep = strlen(rep);
    if (!with)
        with = "";
    len_with = strlen(with);

    ins = orig;
    for (count = 0; tmp = strstr(ins, rep); ++count) {
        ins = tmp + len_rep;
    }

    // first time through the loop, all the variable are set correctly
    // from here on,
    //    tmp points to the end of the result string
    //    ins points to the next occurrence of rep in orig
    //    orig points to the remainder of orig after "end of rep"
    tmp = result = malloc(strlen(orig) + (len_with - len_rep) * count + 1);

    if (!result)
        return NULL;

    while (count--) {
        ins = strstr(orig, rep);
        len_front = ins - orig;
        tmp = strncpy(tmp, orig, len_front) + len_front;
        tmp = strcpy(tmp, with) + len_with;
        orig += len_front + len_rep; // move to next "end of rep"
    }
    strcpy(tmp, orig);
    return result;
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
	*stemmed = str_replace(*stemmed, "formulum", "formula");
	free(morpha_outstream_buff_ptr);

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
