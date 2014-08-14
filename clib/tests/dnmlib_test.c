#include <stdio.h>
#include <string.h>

#include <libxml/parser.h>
#include <libxml/tree.h>

#include "../old_dnmlib.h"


const char TEST_XHTML_DOCUMENT[] = "../../t/documents/1311.6767.xhtml";

int main(void) {
	xmlDoc* doc = xmlReadFile(TEST_XHTML_DOCUMENT, NULL, 0);
	if (doc==NULL) {
		fprintf(stderr, "Couldn't open %s - dnmlib test failing\n", TEST_XHTML_DOCUMENT);
		return 1;
	}

	dnmPtr dnm = createDNM(doc, DNM_NORMALIZE_MATH | DNM_SKIP_CITE);

	dnmIteratorPtr it = getDnmIterator(dnm, DNM_LEVEL_SENTENCE);

	dnmIteratorNext(it);

	dnmIteratorPtr it2 = getDnmChildrenIterator(it);

	dnmIteratorNext(it2);
	dnmIteratorNext(it2);

	char *string = getDnmIteratorContent(it2);
	if (strcmp(string, "Bell")) {
		fprintf(stderr, "dnmlib test failed - expected 'Bell', got '%s'\n", string);
		free(string);
		free(it2);
		free(it);
		freeDNM(dnm);
		xmlFreeDoc(doc);
		xmlCleanupParser();
		return 1;

	}

	free(string);
	free(it2);
	free(it);
	freeDNM(dnm);
	xmlFreeDoc(doc);
	xmlCleanupParser();

	return 0;
}
