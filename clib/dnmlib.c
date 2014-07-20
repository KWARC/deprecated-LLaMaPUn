#include "dnmlib.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>


//Let's do it the "clean" way
#define CHECK_ALLOC(ptr) {if (ptr==NULL) {fprintf(stderr, "Couldn't allocate memory, exiting\n"); exit(1);}}

#define POSSIBLE_RESIZE(ptr, index, oldsizeptr, newsize, type) \
		{if (index >= *oldsizeptr) {\
			ptr = (type*)realloc(ptr, newsize); *oldsizeptr=newsize; CHECK_ALLOC(ptr); }}


//for the creation of the DNM, we need to keep some things in mind:
struct tmp_parsedata {
	size_t plaintext_allocated;
	size_t plaintext_index;

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

char ** getAnnotationList(dnmPtr dnm, char *string) {
	char *i1 = string;
	char *i2 = string;
	int endofannotation = 0;

	char **annotationlist = (char **) malloc(16*sizeof(char *));
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
	return realloc(annotationlist, list_index);
}

inline void copy_into_plaintext(const char *string, dnmPtr dnm, struct tmp_parsedata *dcs) {
	while (*string != '\0') {
		POSSIBLE_RESIZE(dnm->plaintext, dcs->plaintext_index, &dcs->plaintext_allocated,
			dcs->plaintext_allocated*2, char);
		dnm->plaintext[dcs->plaintext_index++] = *string++;
	}
}

void parse_dom_into_dnm(xmlNode *n, dnmPtr dnm, struct tmp_parsedata *dcs, long parameters) {	
	xmlNode *node;
	xmlAttr *attr;
	xmlChar *tmpxmlstr;
	xmlChar *id;
	char **annotationlist;
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
			copy_into_plaintext((char*)tmpxmlstr, dnm, dcs);
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
					annotationlist = getAnnotationList(dnm, (char *)tmpxmlstr);
					xmlFree(tmpxmlstr);
				}
			}
			free(annotationlist);
			xmlFree(id);
			parse_dom_into_dnm(node->children, dnm, dcs, parameters);
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


	//Call the actual parsing function
	parse_dom_into_dnm(xmlDocGetRootElement(doc), dnm, dcs, parameters);


	//----------------CLEAN UP----------------

	//end plaintext with \0 and truncate array
	POSSIBLE_RESIZE(dnm->plaintext, dcs->plaintext_index, &dcs->plaintext_allocated,
			dcs->plaintext_allocated+1, char);
	dnm->plaintext[dcs->plaintext_index++] = '\0';
	dnm->size_plaintext = dcs->plaintext_index;
	dnm->plaintext = realloc(dnm->plaintext, dcs->plaintext_index);
	CHECK_ALLOC(dnm->plaintext);

	free(dcs);

	return dnm;
}

void freeDNM(dnmPtr dnm) {
	struct hash_element_string *current, *tmp;
	HASH_ITER(hh, dnm->annotation_handle, current, tmp) {
		HASH_DEL(dnm->annotation_handle, current);
		free(current->string);
		free(current);
	}

	free(dnm->plaintext);
	free(dnm);
}
