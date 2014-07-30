#include <libxml/tree.h>
#include <uthash.h>

#include <string.h>

//some bit stuff for parameters
#define DNM_NORMALIZE_MATH (1 << 0)
#define DNM_SKIP_MATH      (1 << 1)
#define DNM_SKIP_CITE      (1 << 2)

//different levels for iterators
enum dnm_level {DNM_LEVEL_PARA, DNM_LEVEL_SENTENCE, DNM_LEVEL_WORD, DNM_LEVEL_NONE};

//struct for string items in uthash
struct hash_element_string {
	char *string;
	UT_hash_handle hh;
};


//=======================================================
//Section: Types
//=======================================================


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


//=======================================================
//Section: Functions
//=======================================================


dnmPtr createDNM(xmlDocPtr doc, long parameters);
	/* Creates a DNM and returns a pointer to it.
	   The memory has to be free'd later by calling freeDNM */

void freeDNM(dnmPtr);
	/* frees the DNM */


//ITERATORS

dnmIteratorPtr getDnmIterator(dnmPtr dnm, enum dnm_level level);
	/* returns an iterator over dnm on the specified level
	   memory allocated for the iterator has to be free'd */

dnmIteratorPtr getDnmChildrenIterator(dnmIteratorPtr it);
	/* returns iterator for the children of the current position of it,
	   i.e. over the sentences of a certain paragraph, or over the words of a sentence
	   memory allocated for new iterator has to be free'd manually as well */

int dnmIteratorNext(dnmIteratorPtr it);
	/* makes the iterator point to the next element of the document
	   returns 0, if there is no element left, otherwise 1 */

int dnmIteratorPrevious(dnmIteratorPtr it);
	/* like dnmIteratorNext, just the other direction */

char *getDnmIteratorContent(dnmIteratorPtr it);
	/* returns the plaintext corresponding to the current iterator position.
	   The string, which is ended by \0, has to be freed manually. */


//ANNOTATIONS

int dnmIteratorHasAnnotation(dnmIteratorPtr it, const char *annotation);
	/* Checks, whether the element, pointed to by the iterator, has the annotation.
	   Returns 1, if it has the annotation, otherwise 0.
	   Note that there is a faster way, if you want to repeatedly check for one specific annotation.
	   (by determining aPtr just once, feature isn't implemented yet) */

int dnmIteratorHasAnnotationInherited(dnmIteratorPtr it, const char *annotation);
	/* like dnmIteratorHasAnnotation, just for inherited annotations.
	   Inherited annotations are annotations of the parent nodes */


void dnmIteratorAddAnnotation(dnmIteratorPtr it, const char *annotation, int writeIntoDOM, int inheritToChildren);
	/* writes adds an annotation to the dnm chunk the iterator refers to.
	   The annotation can also be written into the DOM, and inherited to the child chunks.
	   Again, note that there is a faster way for doing this repeatedly with an annotation,
	   by determining aPtr just once. This feature isn't implemented yet */


