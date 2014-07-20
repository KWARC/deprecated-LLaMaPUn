#include <libxml/tree.h>
#include <uthash.h>

#include <string.h>

#define DNM_NORMALIZE_MATH (1 << 0)
#define DNM_SKIP_MATH      (1 << 1)
#define DNM_SKIP_CITE      (1 << 2)


enum dnm_level {DNM_LEVEL_PARA, DNM_LEVEL_SENTENCE, DNM_LEVEL_WORD};


struct hash_element_string {
	char *string;
	UT_hash_handle hh;
};


struct dnm_struct {
	xmlDocPtr document;

	char *plaintext;

	struct hash_element_string *annotation_handle;

	size_t size_plaintext;
};

typedef struct dnm_struct * dnmPtr;

struct dnm_chunk {
	const char *id;
	xmlNodePtr *dom_node;

	enum dnm_level level;
	long offset_parent;
	long offset_children_start;
	long offset_children_end;

	unsigned long offset_start;
	unsigned long offset_end;
};

dnmPtr createDNM(xmlDocPtr doc, long parameters);
void freeDNM(dnmPtr);
