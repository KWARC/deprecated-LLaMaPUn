#include <libxml/tree.h>
#include <uthash.h>

#include <string.h>

#define DNM_NORMALIZE_MATH (1 << 0)
#define DNM_SKIP_MATH      (1 << 1)
#define DNM_SKIP_CITE      (1 << 2)


enum dnm_level {DNM_LEVEL_PARA, DNM_LEVEL_SENTENCE, DNM_LEVEL_WORD, DNM_LEVEL_NONE};


struct hash_element_string {
	char *string;
	UT_hash_handle hh;
};


struct dnm_struct {
	xmlDocPtr document;

	char *plaintext;

	struct hash_element_string *annotation_handle;

	struct dnm_chunk * para_level;
	struct dnm_chunk * sent_level;
	struct dnm_chunk * word_level;

	size_t size_para_level;
	size_t size_sent_level;
	size_t size_word_level;

	size_t size_plaintext;
};

typedef struct dnm_struct * dnmPtr;

struct dnm_chunk {
	char *id;
	xmlNode *dom_node;

	enum dnm_level level;
	long offset_parent;
	long offset_children_start;
	long offset_children_end;

	size_t offset_start;
	size_t offset_end;
};

dnmPtr createDNM(xmlDocPtr doc, long parameters);
void freeDNM(dnmPtr);
