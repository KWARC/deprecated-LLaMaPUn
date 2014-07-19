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
};


inline void copy_into_plaintext(const char *string, struct dnm *newdnm, struct tmp_parsedata *dcs) {
	while (*string != '\0') {
		POSSIBLE_RESIZE(newdnm->plaintext, dcs->plaintext_index, &dcs->plaintext_allocated,
			dcs->plaintext_allocated*2, char);
		newdnm->plaintext[dcs->plaintext_index++] = *string++;
	}
}

void parse_dom_into_dnm(xmlNode *n, struct dnm * newdnm, struct tmp_parsedata *dcs, long parameters) {	
	xmlNode *node;
	xmlAttr *attr;
	for (node = n; node!=NULL; node = node->next) {
		//possibly normalize math tags
		if ((parameters&DNM_NORMALIZE_MATH) && xmlStrEqual(node->name, BAD_CAST "math")) {
			copy_into_plaintext("[MathFormula]", newdnm, dcs);
		}
		//possibly ignore math tags
		else if ((parameters&DNM_SKIP_MATH) && xmlStrEqual(node->name, BAD_CAST "math")) {

		}
		//possibly ignore cite tags
		else if ((parameters&DNM_SKIP_CITE) && xmlStrEqual(node->name, BAD_CAST "cite")) {

		}
		//copy text nodes
		else if (xmlStrEqual(node->name, BAD_CAST "text")) {
			copy_into_plaintext((char*)xmlNodeGetContent(node), newdnm, dcs);
		}
		//otherwise parse children recursively
		else {
			parse_dom_into_dnm(node->children, newdnm, dcs, parameters);
		}
	}
}

struct dnm* createDNM(xmlDocPtr doc, long parameters) {
	//check arguments
	if (doc==NULL) {
		fprintf(stderr, "dnmlib - Didn't get an xmlDoc - parse error??\n");
		return NULL;
	}

	if ((parameters&DNM_NORMALIZE_MATH) && (parameters&DNM_SKIP_MATH)) {
		fprintf(stderr, "dnmlib - got conflicting parameters (skip and normalize math)\n");
	}

	//----------------INITIALIZE----------------
	struct dnm* newdnm = (struct dnm*)malloc(sizeof(struct dnm));
	CHECK_ALLOC(newdnm);
	struct tmp_parsedata *dcs = (struct tmp_parsedata*)malloc(sizeof(struct tmp_parsedata));
	CHECK_ALLOC(dcs);

	//set initial values, and allocate initial memory
	newdnm->document = doc;
	newdnm->plaintext = (char*)malloc(sizeof(char)*4096);
	CHECK_ALLOC(newdnm->plaintext);

	dcs->plaintext_allocated = 4096;
	dcs->plaintext_index = 0;


	//Call the actual parsing function
	parse_dom_into_dnm(xmlDocGetRootElement(doc), newdnm, dcs, parameters);


	//----------------CLEAN UP----------------

	//end plaintext with \0 and truncate array
	POSSIBLE_RESIZE(newdnm->plaintext, dcs->plaintext_index, &dcs->plaintext_allocated,
			dcs->plaintext_allocated+1, char);
	newdnm->plaintext[dcs->plaintext_index++] = '\0';
	newdnm->size_plaintext = dcs->plaintext_index;
	newdnm->plaintext = realloc(newdnm->plaintext, dcs->plaintext_index);
	CHECK_ALLOC(newdnm->plaintext);

	free(dcs);

	return newdnm;
}

void freeDNM(struct dnm* d) {
	free(d->plaintext);
	free(d);
}
