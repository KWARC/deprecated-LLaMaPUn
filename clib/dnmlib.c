#include "dnmlib.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>


//macro, checking whether memory was allocated successfully
#define CHECK_ALLOC(ptr) {if (ptr==NULL) {fprintf(stderr, "Couldn't allocate memory, exiting\n"); fflush(stderr); exit(1);}}

//resize an array if the index exceeds the old size
#define POSSIBLE_RESIZE(ptr, index, oldsizeptr, newsize, type) \
		{if (index >= *oldsizeptr) {\
			ptr = (type*)realloc(ptr, (newsize)*sizeof(type)); *oldsizeptr=newsize; CHECK_ALLOC(ptr); }}

//=======================================================
//Section: Tokenization
//=======================================================

void markSentences(dnmPtr dnm, size_t start_offsets[], size_t end_offsets[], size_t n) {
	//to be implemented
}


//=======================================================
//Section: Creating DNM
//=======================================================


//for the creation of the DNM, we need to keep some things in mind:
struct tmp_parsedata {
	size_t plaintext_allocated;
	size_t plaintext_index;
};


void copy_into_plaintext(const char *string, dnmPtr dnm, struct tmp_parsedata *dcs) {
	/* appends string to the plaintext, stored in dnm */
	while (*string != '\0') {
		POSSIBLE_RESIZE(dnm->plaintext, dcs->plaintext_index, &dcs->plaintext_allocated,
			dcs->plaintext_allocated*2, char);
		dnm->plaintext[dcs->plaintext_index++] = *string++;
	}
}


void parse_dom_into_dnm(xmlNode *n, dnmPtr dnm, struct tmp_parsedata *dcs, long parameters) {	
	/* the core function which (recursively) parses the DOM into the DNM */

	//declaring a lot of variables required later on...
	xmlNode *node;
	xmlChar *tmpxmlstr;

	//iterate over nodes
	for (node = n; node!=NULL; node = node->next) {
		//possibly normalize math tags
		if ((parameters&DNM_NORMALIZE_TAGS) && xmlStrEqual(node->name, BAD_CAST "math")) {
			copy_into_plaintext("[MathFormula]", dnm, dcs);
		}
		//possibly normalize cite tags
		else if ((parameters&DNM_NORMALIZE_TAGS) && xmlStrEqual(node->name, BAD_CAST "cite")) {
			copy_into_plaintext("[CiteExpression]", dnm, dcs);
		}
		//copy contents of text nodes into plaintext
		else if (xmlStrEqual(node->name, BAD_CAST "text")) {

			tmpxmlstr = xmlNodeGetContent(node);
			CHECK_ALLOC(tmpxmlstr);
			copy_into_plaintext((char*)tmpxmlstr, dnm, dcs);
			xmlFree(tmpxmlstr);
		}
		//otherwise, parse children recursively
		else {
			parse_dom_into_dnm(node->children, dnm, dcs, parameters);
		}
	}
}

dnmPtr createDNM(xmlDocPtr doc, long parameters) {
	/* Creates a DNM and returns a pointer to it.
	   The memory has to be free'd later by calling freeDNM */

	//check arguments
	if (doc==NULL) {
		fprintf(stderr, "dnmlib - Didn't get an xmlDoc - parse error??\n");
		return NULL;
	}

	//----------------INITIALIZE----------------
	//dcs (data required only during parsing)
	struct tmp_parsedata *dcs = (struct tmp_parsedata*)malloc(sizeof(struct tmp_parsedata));
	CHECK_ALLOC(dcs);

	//dnm
	dnmPtr dnm = (dnmPtr)malloc(sizeof(struct dnm_struct));
	CHECK_ALLOC(dnm);
	dnm->parameters = parameters;

	dnm->document = doc;

	//plaintext
	dnm->plaintext = (char*)malloc(sizeof(char)*4096);
	CHECK_ALLOC(dnm->plaintext);

	dcs->plaintext_allocated = 4096;
	dcs->plaintext_index = 0;


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

	//we don't need the dcs anymore
	free(dcs);

	return dnm;
}



//=======================================================
//Section: Freeing DNM
//=======================================================

void freeDNM(dnmPtr dnm) {
	/* frees the dnm */

	//free plaintext
	free(dnm->plaintext);
	//free level lists
	free(dnm);
}
