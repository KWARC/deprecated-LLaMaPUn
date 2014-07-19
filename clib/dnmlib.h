#include <libxml/tree.h>
#include <string.h>

#define DNM_NORMALIZE_MATH (1 << 0)
#define DNM_SKIP_MATH      (1 << 1)
#define DNM_SKIP_CITE      (1 << 2)


enum dnm_level {DNM_LEVEL_PARA, DNM_LEVEL_SENTENCE, DNM_LEVEL_WORD};


struct dnm {
	xmlDocPtr document;

	char *plaintext;


	size_t size_plaintext;
};

struct dnm_chunk {
	const char *id;
	//char **annotations;
	//unsigned number_of_annotations;
	xmlNodePtr *dom_node;

	enum dnm_level level;
	long offset_parent;
	long offset_children_start;
	long offset_children_end;

	unsigned long offset_start;
	unsigned long offset_end;
};

struct dnm* createDNM(xmlDocPtr doc, long parameters);
void freeDNM(struct dnm*);

