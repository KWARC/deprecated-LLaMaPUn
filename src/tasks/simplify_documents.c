#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <ftw.h>
#include <unistd.h>
#include <math.h>

#include "llamapun/document_loader.h"
#include "llamapun/utils.h"
#include "llamapun/unicode_normalizer.h"

#include <uthash.h>
#include <libxml/parser.h>


FILE *OUTFILE = NULL;

int FILE_COUNTER = 0;

const char const *paragraph_xpath = "//*[local-name()='section' and @class='ltx_section']//*[local-name()='div' and @class='ltx_para']";
const char const *relaxed_paragraph_xpath = "//*[local-name()='div' and @class='ltx_para']";

struct document_frequencies_hash* WORD_TO_ID = NULL;
int ID_COUNTER = 0;
char *destination_dir;

int is_definition(xmlNode * n) {
	if (!xmlStrEqual(n->name, BAD_CAST "div")) return 0;
	xmlAttr *attr;
	char *class_value;
	int retval = 0;
	for (attr = n->properties; attr != NULL; attr = attr->next) {
		if (xmlStrEqual(attr->name, BAD_CAST "class")) {
			class_value = (char*) xmlNodeGetContent(attr->children);
			if(strstr(class_value, "ltx_theorem_def") != NULL) 
				retval = 1;
			xmlFree(class_value);
			return retval;
		}
	}
	return 0;   // has no class
}


void process_paragraph(char *words[], size_t number_of_words, xmlNodePtr node) {
	if (!number_of_words) return;    //We're not interested in empty paragraphes
	fprintf(OUTFILE, "%d", is_definition(node->parent) ? 1 : -1);
	struct document_frequencies_hash* entry = NULL;
	size_t i;
	for (i = 0; i < number_of_words; i++) {
		HASH_FIND_STR(WORD_TO_ID, words[i], entry);   //find word
		if (entry == NULL) {
			//create entry
			entry = (struct document_frequencies_hash*)malloc(sizeof(struct document_frequencies_hash));
			entry->word = strdup(words[i]);
			entry->count = ID_COUNTER++;
			HASH_ADD_KEYPTR(hh, WORD_TO_ID, entry->word, strlen(entry->word), entry);
		}
		fprintf(OUTFILE, " %d", entry->count);   //print word ID
	}
	fprintf(OUTFILE, "\n");
}



int parse(const char *filename, const struct stat *s, int type) {
	if (type != FTW_F) return 0; //Not a file
	UNUSED(s);

	printf("File counter: %d\n", FILE_COUNTER++);

	//construct new file name:
	char name[2048];
	snprintf(name, sizeof(name), "%s", destination_dir);
	char *c = name;
	while (*c) c++;   //end of destination dir
	if (*(c-1) != '/') {
		*c++ = '/';   //append a slash
	}
	//find end beginning of file name
	char *d = filename + strlen(filename) - 1;
	while (*d != '/') d--;
	snprintf(c, sizeof(name) - (c - name), "%s.simple", d+1);  //append file name
	OUTFILE = fopen(name, "w");
	if (OUTFILE == NULL) {
		fprintf(stderr, "Couldn't write to %s\n", name);
		exit(1);
	}
	
	printf("%s  =>  %s\n", filename, name);

	xmlDoc *document = read_document(filename);
	if (document == NULL) {
		fprintf(stderr, "Couldn't load document %s\n", filename);
	}
	unicode_normalize_dom(document);
	int b = with_words_at_xpath(process_paragraph, document, paragraph_xpath, /* logfile = */ stderr,
			WORDS_NORMALIZE_WORDS | WORDS_STEM_WORDS | WORDS_MARK_END_OF_SENTENCE,
			DNM_NORMALIZE_TAGS | DNM_IGNORE_LATEX_NOTES);
	if (!b) {
		with_words_at_xpath(process_paragraph, document, relaxed_paragraph_xpath, /* logfile = */ stderr,
			WORDS_NORMALIZE_WORDS | WORDS_STEM_WORDS | WORDS_MARK_END_OF_SENTENCE,
			DNM_NORMALIZE_TAGS | DNM_IGNORE_LATEX_NOTES);
	}
	fclose(OUTFILE);
	xmlFreeDoc(document);
	return 0;
}


int main(int argc, char *argv[]) {
	printf("Usage: ./simplify_documents source_dir/ destination_dir/\n");
	destination_dir = argv[2];
	init_document_loader();
	ftw(argv[1], parse, 1);
	close_document_loader();

	FILE *map = fopen("map.txt", "w");
	if (map == NULL) {
		fprintf(stderr, "Couldn't write to map.txt.\n");
		free_document_frequencies_hash(WORD_TO_ID);
	} else {
		//write id's into file and free hash
		struct document_frequencies_hash *a, *tmp;
		HASH_ITER(hh, WORD_TO_ID, a, tmp) {
			HASH_DEL(WORD_TO_ID, a);
			fprintf(map, "%d %s\n", a->count, a->word);
			free(a->word);
			free(a);
		}
		fclose(map);
	}
	return 0;
}


