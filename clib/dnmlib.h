#include <libxml/tree.h>
#include <string.h>

#define DNF_NORMALIZE_MATH (1 << 0)
#define DNF_SKIP_MATH      (1 << 1)
#define DNF_SKIP_CITE      (1 << 2)


struct dnm {
	xmlDocPtr document;

	char *plaintext;


	size_t size_plaintext;
};

struct dnm* createDNM(xmlDocPtr doc, long parameters);
void freeDNM(struct dnm*);

