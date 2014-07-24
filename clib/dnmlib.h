#include <libxml/tree.h>
#include <uthash.h>

#include <string.h>

#define DNM_NORMALIZE_MATH (1 << 0)
#define DNM_SKIP_MATH      (1 << 1)
#define DNM_SKIP_CITE      (1 << 2)


enum dnm_level {DNM_LEVEL_PARA, DNM_LEVEL_SENTENCE, DNM_LEVEL_WORD, DNM_LEVEL_NONE};

//struct for string items in uthash
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

//one chunk corresponds to one paragraph, sentence, or word
struct dnm_chunk {
	char *id;
	xmlNode *dom_node;

	enum dnm_level level;
	long offset_parent;
	long offset_children_start;
	long offset_children_end;

	char **annotations;
	size_t number_of_annotations;
	size_t annotations_allocated;

	char **inherited_annotations;
	size_t number_of_inherited_annotations;
	size_t inherited_annotations_allocated;

	size_t offset_start;
	size_t offset_end;
};

struct dnm_iterator {
	dnmPtr dnm;
	enum dnm_level level;
	size_t pos;
	size_t start;
	size_t end;
};

typedef struct dnm_iterator * dnmIteratorPtr;

dnmPtr createDNM(xmlDocPtr doc, long parameters);
void freeDNM(dnmPtr);

dnmIteratorPtr getDnmIterator(dnmPtr dnm, enum dnm_level level);  //allocates memory using malloc => has to be free'd manually
dnmIteratorPtr getDnmChildrenIterator(dnmIteratorPtr it);  //returns iterator over children, has to be freed (NULL level has no children)
int dnmIteratorNext(dnmIteratorPtr it);
int dnmIteratorPrevious(dnmIteratorPtr it);

char *getDnmIteratorContent(dnmIteratorPtr it);    //returned array (\0-terminated) has to be free'd manually
int dnmIteratorHasAnnotation(dnmIteratorPtr it, const char *annotation);   //1 if it has the annotation, 0 otherwise
int dnmIteratorHasAnnotationInherited(dnmIteratorPtr it, const char *annotation);

void dnmIteratorAddAnnotation(dnmIteratorPtr it, const char *annotation, int writeIntoDOM, int inheritToChildren);
