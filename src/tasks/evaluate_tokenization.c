#include <stdio.h>
#include <stdlib.h>

#include "llamapun/document_loader.h"
#include "llamapun/utils.h"

#include <libxml/parser.h>



char *paragraph_xpath = "//*[local-name()='section' and @class='ltx_section']//*[local-name()='div' and @class='ltx_para']";


void print_words_of_paragraph(char *words[], size_t number) {
	size_t i;
	for (i = 0; i < number; i++) {
		printf("%s\n", words[i]);
	}
	printf("\nEND OF PARAGRAPH\n\n");
}


int main(int argc, char const *args[]) {
	if (argc != 2) {
		printf("Please provide a file name as an argument\n");
		exit(1);
	}

	xmlDocPtr document = read_document(args[1]);
	init_document_loader();

	get_words_of_xpath(document, paragraph_xpath, print_words_of_paragraph,
			WORDS_NORMALIZE_NUMBERS | WORDS_STEM_WORDS | WORDS_MARK_END_OF_SENTENCE,
			/* logfile = */ stderr, DNM_NORMALIZE_TAGS | DNM_IGNORE_LATEX_NOTES);

	xmlFreeDoc(document);
	close_document_loader();
	return 0;
}