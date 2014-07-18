#include "dnmlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>


//Let's do it the "clean" way
#define CHECK_ALLOC(ptr) {if (ptr==NULL) {fprintf(stderr, "Couldn't allocate memory, exiting\n"); exit(1);}}



//for the creation of the DNM, we need to keep some things in mind:
struct tmp_parsedata {
	FILE *plaintext_stream;
};


void parse_dom_into_dnm(xmlNode *n, struct dnm * newdnm, struct tmp_parsedata *dcs, long parameters) {
	xmlNode *node;
	xmlAttr *attr;
	for (node = n; node!=NULL; node = node->next) {
		//possibly normalize math tags
		if ((parameters&DNF_NORMALIZE_MATH) && xmlStrEqual(node->name, BAD_CAST "math")) {
			fprintf(dcs->plaintext_stream, "[MathFormula]");
		}
		//possibly ignore math tags
		else if ((parameters&DNF_SKIP_MATH) && xmlStrEqual(node->name, BAD_CAST "math")) {

		}
		//possibly ignore cite tags
		else if ((parameters&DNF_SKIP_CITE) && xmlStrEqual(node->name, BAD_CAST "cite")) {

		}
		//copy text nodes
		else if (xmlStrEqual(node->name, BAD_CAST "text")) {
			fprintf(dcs->plaintext_stream, "%s", (char*)xmlNodeGetContent(node));
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

	if ((parameters&DNF_NORMALIZE_MATH) && (parameters&DNF_SKIP_MATH)) {
		fprintf(stderr, "dnmlib - got conflicting parameters (skip and normalize math)\n");
	}

	//getting started
	struct dnm* newdnm = (struct dnm*)malloc(sizeof(struct dnm));
	CHECK_ALLOC(newdnm);
	struct tmp_parsedata *dcs = (struct tmp_parsedata*)malloc(sizeof(struct tmp_parsedata));
	CHECK_ALLOC(dcs);

	//set initial values, and allocate initial memory
	newdnm->document = doc;

	dcs->plaintext_stream = open_memstream(&newdnm->plaintext, &newdnm->size_plaintext);

	//Call the actual parsing function
	parse_dom_into_dnm(xmlDocGetRootElement(doc), newdnm, dcs, parameters);

	//clean up
	fclose(dcs->plaintext_stream);
	CHECK_ALLOC(newdnm->plaintext);

	free(dcs);

	return newdnm;
}

void freeDNM(struct dnm* d) {
	free(d->plaintext);
	free(d);
}