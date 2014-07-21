#include "dnmlib.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>


//Let's do it the "clean" way
#define CHECK_ALLOC(ptr) {if (ptr==NULL) {fprintf(stderr, "Couldn't allocate memory, exiting\n"); fflush(stderr); exit(1);}}

#define POSSIBLE_RESIZE(ptr, index, oldsizeptr, newsize, type) \
		{if (index >= *oldsizeptr) {\
			ptr = (type*)realloc(ptr, newsize*sizeof(type)); *oldsizeptr=newsize; CHECK_ALLOC(ptr); }}


//for the creation of the DNM, we need to keep some things in mind:
struct tmp_parsedata {
	size_t plaintext_allocated;
	size_t plaintext_index;

	size_t para_level_allocated;
	size_t sent_level_allocated;
	size_t word_level_allocated;

	size_t para_level_index;
	size_t sent_level_index;
	size_t word_level_index;

	char * para_annotation;
	char * sent_annotation;
	char * word_annotation;
};


inline char * getAnnotationPtr(dnmPtr dnm, const char *annotation, int create) {
	struct hash_element_string *tmp;
	HASH_FIND_STR(dnm->annotation_handle, annotation, tmp);
	if (tmp == NULL) {		//Couldn't find annotation
		if (create) {
			tmp = (struct hash_element_string *)malloc(sizeof(struct hash_element_string));
			CHECK_ALLOC(tmp);
			tmp->string = strdup(annotation);
			CHECK_ALLOC(tmp->string);
			HASH_ADD_KEYPTR(hh, dnm->annotation_handle, tmp->string, strlen(tmp->string), tmp);
			return tmp->string;
		} else return NULL;
	} else {
		return tmp->string;
	}
}

char ** getAnnotationList(dnmPtr dnm, char *string, size_t * final_length) {
	char *i1 = string;
	char *i2 = string;
	int endofannotation = 0;

	char **annotationlist = (char **) malloc(16*sizeof(char *));
	CHECK_ALLOC(annotationlist);
	size_t list_length = 16;
	size_t list_index = 0;

	while (!endofannotation) {
		//go ahead to end of word
		while (*i2 != ' ' && *i2 != '\0') {
			i2++;
		}
		if (*i2 == '\0') endofannotation = 1;
		POSSIBLE_RESIZE(annotationlist, list_index, &list_length, list_length*2, char *);
		annotationlist[list_index++] = getAnnotationPtr(dnm, i1, 1);
		i1 = ++i2;
	}
	//truncate
	*final_length = list_index;
	return realloc(annotationlist, list_index * sizeof(char));
}

inline void copy_into_plaintext(const char *string, dnmPtr dnm, struct tmp_parsedata *dcs) {
	while (*string != '\0') {
		POSSIBLE_RESIZE(dnm->plaintext, dcs->plaintext_index, &dcs->plaintext_allocated,
			dcs->plaintext_allocated*2, char);
		dnm->plaintext[dcs->plaintext_index++] = *string++;
	}
}

enum dnm_level getLevelFromAnnotations(struct tmp_parsedata *dcs, char **annotationlist,
                                       size_t annotationlist_length) {
	char *tmp;
	while (annotationlist_length) {
		tmp = annotationlist[--annotationlist_length];
		if (tmp == dcs->para_annotation) return DNM_LEVEL_PARA;
		if (tmp == dcs->sent_annotation) return DNM_LEVEL_SENTENCE;
		if (tmp == dcs->word_annotation) return DNM_LEVEL_WORD;
	}
	return DNM_LEVEL_NONE;
}

void parse_dom_into_dnm(xmlNode *n, dnmPtr dnm, struct tmp_parsedata *dcs, long parameters) {	
	xmlNode *node;
	xmlAttr *attr;
	xmlChar *tmpxmlstr;
	xmlChar *id;
	char **annotationlist;
	size_t annotationlist_length;
	enum dnm_level level;
	struct dnm_chunk * current_chunk;
	size_t level_offset;
	for (node = n; node!=NULL; node = node->next) {
		//possibly normalize math tags
		if ((parameters&DNM_NORMALIZE_MATH) && xmlStrEqual(node->name, BAD_CAST "math")) {
			copy_into_plaintext("[MathFormula]", dnm, dcs);
		}
		//possibly ignore math tags
		else if ((parameters&DNM_SKIP_MATH) && xmlStrEqual(node->name, BAD_CAST "math")) {

		}
		//possibly ignore cite tags
		else if ((parameters&DNM_SKIP_CITE) && xmlStrEqual(node->name, BAD_CAST "cite")) {

		}
		//copy text nodes
		else if (xmlStrEqual(node->name, BAD_CAST "text")) {

			tmpxmlstr = xmlNodeGetContent(node);
			CHECK_ALLOC(tmpxmlstr);
			xmlFree(tmpxmlstr);
		}
		//otherwise parse children recursively
		else {
			id = NULL;
			annotationlist = NULL;
			


			for (attr = node->properties; attr != NULL; attr = attr->next) {

				if (xmlStrEqual(attr->name, BAD_CAST "id")) {
					if (id) fprintf(stderr, "Error: Multiple id attributes in one tag\n");
					id = xmlNodeGetContent(attr->children);
					CHECK_ALLOC(id);
				} else if (xmlStrEqual(attr->name, BAD_CAST "class")) {
					if (annotationlist) fprintf(stderr, "Error: Multiple class attributes in one tag\n");
					tmpxmlstr = xmlNodeGetContent(attr->children);
					CHECK_ALLOC(tmpxmlstr);
					annotationlist = getAnnotationList(dnm, (char *)tmpxmlstr, &annotationlist_length);
					CHECK_ALLOC(annotationlist);
					xmlFree(tmpxmlstr);
				}
			}
			if (annotationlist == NULL) {
				parse_dom_into_dnm(node->children, dnm, dcs, parameters);
			} else {
				level = getLevelFromAnnotations(dcs, annotationlist, annotationlist_length);
				current_chunk = NULL;
				switch (level) {
					case DNM_LEVEL_PARA:
						POSSIBLE_RESIZE(dnm->para_level, dcs->para_level_index, &dcs->para_level_allocated,
						                dcs->para_level_allocated*2, struct dnm_chunk);
						level_offset = dcs->para_level_index++;
						current_chunk = (dnm->para_level) + level_offset;
						break;
					case DNM_LEVEL_SENTENCE:
						POSSIBLE_RESIZE(dnm->sent_level, dcs->sent_level_index, &dcs->sent_level_allocated,
						                dcs->sent_level_allocated*2, struct dnm_chunk);
						level_offset = dcs->sent_level_index++;
						current_chunk = (dnm->sent_level) + level_offset;
						break;
					case DNM_LEVEL_WORD:
						POSSIBLE_RESIZE(dnm->word_level, dcs->word_level_index, &dcs->word_level_allocated,
						                dcs->word_level_allocated*2, struct dnm_chunk);
						level_offset = dcs->word_level_index++;
						current_chunk = (dnm->word_level) + level_offset;
						break;
					case DNM_LEVEL_NONE:
						current_chunk = NULL;
						break;
				}

				if (current_chunk != NULL) {
					current_chunk->id = (id == NULL ? NULL : strdup((char*)id));
					current_chunk->dom_node = node;
					current_chunk->offset_start = dcs->plaintext_index;

					parse_dom_into_dnm(node->children, dnm, dcs, parameters);

					//due to realloc, location of current_chunk might change :D
					switch (level) {
						case DNM_LEVEL_PARA:
							current_chunk = (dnm->para_level) + level_offset;
							break;
						case DNM_LEVEL_SENTENCE:
							current_chunk = (dnm->sent_level) + level_offset;
							break;
						case DNM_LEVEL_WORD:
							current_chunk = (dnm->word_level) + level_offset;
							break;
						default: break;		//should never occur, but I don't like compiler warnings
					}
					current_chunk->offset_end = dcs->plaintext_index;
				} else {
					parse_dom_into_dnm(node->children, dnm, dcs, parameters);
				}
			}


			free(annotationlist);

			xmlFree(id);
		}
	}
}

dnmPtr createDNM(xmlDocPtr doc, long parameters) {
	//check arguments
	if (doc==NULL) {
		fprintf(stderr, "dnmlib - Didn't get an xmlDoc - parse error??\n");
		return NULL;
	}

	if ((parameters&DNM_NORMALIZE_MATH) && (parameters&DNM_SKIP_MATH)) {
		fprintf(stderr, "dnmlib - got conflicting parameters (skip and normalize math)\n");
	}

	//----------------INITIALIZE----------------
	//dcs (data required only during parsing)
	struct tmp_parsedata *dcs = (struct tmp_parsedata*)malloc(sizeof(struct tmp_parsedata));
	CHECK_ALLOC(dcs);

	//dnm
	dnmPtr dnm = (dnmPtr)malloc(sizeof(struct dnm_struct));
	CHECK_ALLOC(dnm);

	dnm->document = doc;

	//plaintext
	dnm->plaintext = (char*)malloc(sizeof(char)*4096);
	CHECK_ALLOC(dnm->plaintext);

	dcs->plaintext_allocated = 4096;
	dcs->plaintext_index = 0;

	//annotations
	dnm->annotation_handle = NULL;

	dcs->para_annotation = getAnnotationPtr(dnm, "ltx_para", 1);
	dcs->sent_annotation = getAnnotationPtr(dnm, "ltx_sentence", 1);
	dcs->word_annotation = getAnnotationPtr(dnm, "ltx_word", 1);

	//levels
	dnm->para_level = (struct dnm_chunk *) malloc(sizeof(struct dnm_chunk)*32);
	dcs->para_level_allocated = 32;
	dcs->para_level_index = 0;

	dnm->sent_level = (struct dnm_chunk *) malloc(sizeof(struct dnm_chunk)*128);
	dcs->sent_level_allocated = 128;
	dcs->sent_level_index = 0;

	dnm->word_level = (struct dnm_chunk *) malloc(sizeof(struct dnm_chunk)*1024);
	dcs->word_level_allocated = 1024;
	dcs->word_level_index = 0;


	//Call the actual parsing function
	parse_dom_into_dnm(xmlDocGetRootElement(doc), dnm, dcs, parameters);


	//----------------CLEAN UP----------------

	//end plaintext with \0 and truncate array
	POSSIBLE_RESIZE(dnm->plaintext, dcs->plaintext_index, &dcs->plaintext_allocated,
			dcs->plaintext_allocated+1, char);
	dnm->plaintext[dcs->plaintext_index++] = '\0';
	dnm->size_plaintext = dcs->plaintext_index;
	dnm->plaintext = realloc(dnm->plaintext, dcs->plaintext_index * sizeof(char));
	if (dnm->size_plaintext) CHECK_ALLOC(dnm->plaintext);

	//truncate level lists
	dnm->size_para_level = dcs->para_level_index;
	dnm->para_level = realloc(dnm->para_level, dnm->size_para_level * sizeof(struct dnm_chunk));
	if (dnm->size_para_level) CHECK_ALLOC(dnm->para_level);    //only check if memory has actually been allocated
	dnm->size_sent_level = dcs->sent_level_index;
	dnm->sent_level = realloc(dnm->sent_level, dnm->size_sent_level * sizeof(struct dnm_chunk));
	if (dnm->size_sent_level) CHECK_ALLOC(dnm->sent_level);
	dnm->size_word_level = dcs->word_level_index;
	dnm->word_level = realloc(dnm->word_level, dnm->size_word_level * sizeof(struct dnm_chunk));
	if (dnm->size_word_level) CHECK_ALLOC(dnm->word_level);

	free(dcs);

	return dnm;
}

void freeLevelList(struct dnm_chunk * array, size_t size) {
	while (size--) {
		free(array[size].id);
		//to be done: free annotations (once they're actually stored)
	}
	free(array);
}

void freeDNM(dnmPtr dnm) {
	//free annotations
	struct hash_element_string *current, *tmp;
	HASH_ITER(hh, dnm->annotation_handle, current, tmp) {
		HASH_DEL(dnm->annotation_handle, current);
		free(current->string);
		free(current);
	}
	//free plaintext
	free(dnm->plaintext);
	//free level lists
	freeLevelList(dnm->para_level, dnm->size_para_level);
	freeLevelList(dnm->sent_level, dnm->size_sent_level);
	freeLevelList(dnm->word_level, dnm->size_word_level);

	free(dnm);
}
