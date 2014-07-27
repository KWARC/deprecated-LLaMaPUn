#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <uthash.h>
#include <libxml/tree.h>


//macro, checking whether memory was allocated successfully
#define CHECK_ALLOC(ptr) {if (ptr==NULL) {fprintf(stderr, "Couldn't allocate memory, exiting\n"); fflush(stderr); exit(1);}}

//resize an array if the index exceeds the old size
#define POSSIBLE_RESIZE(ptr, index, oldsizeptr, newsize, type) \
		{if (index >= *oldsizeptr) {\
			ptr = (type*)realloc(ptr, (newsize)*sizeof(type)); *oldsizeptr=newsize; CHECK_ALLOC(ptr); }}



//=======================================================
//Section: Tag normalization stuff
//=======================================================


struct tmp_preprocessing_data {
	char *plaintext;
	size_t pt_size;
	size_t pt_index;
};

struct hash_item_tag_normalization {
	char *tag_name;
	char *norm_string;
	UT_hash_handle hh;
};

struct hash_item_tag_normalization * tag_norm_handle = NULL;

#define ADD_TAG_NORMALIZATION(name, repl) {tmp = (struct hash_item_tag_normalization *) malloc(sizeof(struct hash_item_tag_normalization)); \
                                        CHECK_ALLOC(tmp); tmp->tag_name = name; tmp->norm_string = repl;\
                                        HASH_ADD_KEYPTR(hh, tag_norm_handle, tmp->tag_name, strlen(tmp->tag_name), tmp);}

void load_tag_repl_hash() {
	if (tag_norm_handle) return;   //it's loaded already
	struct hash_item_tag_normalization * tmp;
	ADD_TAG_NORMALIZATION("math", "[MathFormula]");
	ADD_TAG_NORMALIZATION("table", "")
	ADD_TAG_NORMALIZATION("script", "");
	ADD_TAG_NORMALIZATION("title", "");
	ADD_TAG_NORMALIZATION("head", "");
}

#undef ADD_TAG_NORMALIZATION

char *getNormalization(const char *tag_name) {
	struct hash_item_tag_normalization *tmp;
	HASH_FIND_STR(tag_norm_handle, tag_name, tmp);
	if (tmp) return tmp->norm_string;
	else return NULL;   //Don't normalize
}


//=======================================================
//Section: Core functionality
//=======================================================

void append_to_plaintext(struct tmp_preprocessing_data *tpd, const char *string) {
	while (*string != '\0') {
		POSSIBLE_RESIZE(tpd->plaintext, tpd->pt_index, &tpd->pt_size,
			tpd->pt_size*2, char);
		tpd->plaintext[tpd->pt_index++] = *string++;
	}
}


void preprocessing_parse(xmlNode *n, struct tmp_preprocessing_data *tpd) {
	xmlNode *node;
	char *normalization;
	xmlChar *content;
	for (node = n; node!=NULL; node = node->next) {
		if (xmlStrEqual(node->name, BAD_CAST "text")) {
			content = xmlNodeGetContent(node);
			append_to_plaintext(tpd, (char *)content);
			xmlFree(content);
		} else {
			normalization = getNormalization((char *)node->name);
			if (normalization) {
				append_to_plaintext(tpd, normalization);
			} else {
				preprocessing_parse(node->children, tpd);
			}
			
		}
	}
}


char * preprocess(xmlDocPtr doc) {
	load_tag_repl_hash();

	struct tmp_preprocessing_data *tpd = (struct tmp_preprocessing_data *) malloc(sizeof(struct tmp_preprocessing_data));
	CHECK_ALLOC(tpd);

	tpd->plaintext = (char *) malloc(sizeof(char)*2048);
	CHECK_ALLOC(tpd->plaintext);
	tpd->pt_index = 0;
	tpd->pt_size = 2048;


	preprocessing_parse(xmlDocGetRootElement(doc), tpd);


	char *result = realloc(tpd->plaintext, tpd->pt_index+1);
	CHECK_ALLOC(result);
	result[tpd->pt_index] = '\0';
	free(tpd);
	return result;
}




//=======================================================
//Section: Freeing memory and cleaning up
//=======================================================



void preprocessorCleanUp() {
	if (tag_norm_handle) {
		struct hash_item_tag_normalization *current, *tmp;
		HASH_ITER(hh, tag_norm_handle, current, tmp) {
			HASH_DEL(tag_norm_handle, current);
			//strings are not malloc'd
			free(current);
		}
		tag_norm_handle = NULL;
	}
}
