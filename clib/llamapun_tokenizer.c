//After installation of llamapun it can be compiled by
//gcc llamapun_tokenizer.c -o tokenizer -lxml2 -lllamapun -I/usr/include/libxml2

#include <stdio.h>
#include <stdlib.h>

#include <libxml/parser.h>
#include <libxml/tree.h>
#include "old_dnmlib.h"

int main(void) {
	//load example document
	xmlDocPtr mydoc = xmlReadFile("../t/documents/1311.0066.xhtml", NULL, 0);

	//Create DNM, with normalized math tags, and ignoring cite tags
	old_dnmPtr mydnm = old_createDNM(mydoc, DNM_NORMALIZE_MATH | DNM_SKIP_CITE | DNM_EXTEND_PARA_RANGE);

	printf("Plaintext: %s", mydnm->plaintext);


	//have some arrays of offsets

	size_t *offset_starts_array;
	size_t *offset_ends_array;
	//last argument is number of sentences
	//markSentences(mydnm, offset_starts_array, offset_ends_array, 0);	

	//clean up
	old_freeDNM(mydnm);
	xmlFreeDoc(mydoc);
	xmlCleanupParser();
	
	return 0;
}
