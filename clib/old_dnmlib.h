/*! \defgroup old_dnmlib old DNMlib
    Most NLP tools work on plain text.
  However, the XML structure contains useful information about the
  structure of a document etc.
  So one tends to switch back and forth.
  The purpose of this library is to simplify this switching
  by providing a DNM (document narrative model) and some tools
  to iterate over it etc.
  @file
*/

#include <libxml/tree.h>
#include <uthash.h>

#include <string.h>

/*! normalize math tags in document */
#define DNM_NORMALIZE_MATH (1 << 0)
/*! skip, i.e. ignore, math tags in document */
#define DNM_SKIP_MATH      (1 << 1)
/*! skip, i.e. ignore, cite tags in document */
#define DNM_SKIP_CITE      (1 << 2)
/*! include title, authors, toc, bibliography, ... in list of paragraphs */
#define DNM_EXTEND_PARA_RANGE (1 << 3)

/*! the different levels for iterators */
enum dnm_level {DNM_LEVEL_PARA, DNM_LEVEL_SENTENCE, DNM_LEVEL_WORD, DNM_LEVEL_NONE};

/*! string element for uthash */
struct hash_element_string {
  char *string;
  UT_hash_handle hh;
};


//=======================================================
//Section: Types
//=======================================================


struct old_dnm_struct {
  xmlDocPtr document;
  long parameters;

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

typedef struct {
  size_t start;
  size_t end;
} old_dnm_offset;

typedef struct old_dnm_offsets_struct {
  size_t start;
  size_t end;
  struct old_dnm_offsets_struct *next;
} old_dnm_offsets;

typedef struct old_dnm_struct * old_dnmPtr;

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
  old_dnmPtr dnm;
  enum dnm_level level;
  size_t pos;
  size_t start;
  size_t end;
};

typedef struct dnm_iterator * dnmIteratorPtr;


//=======================================================
//Section: Functions
//=======================================================

/*! creates a DNM

    Example call: old_createDNM(mydoc, DNM_NORMALIZE_MATH | DNM_SKIP_CITE);
  Memory has to be freed later using old_freeDNM
  @see old_freeDNM
    @param doc a pointer to the DOM
  @param parameters the parameters
  @retval a pointer to the new DNM
*/
old_dnmPtr old_createDNM(xmlDocPtr doc, long parameters);

/*! frees the DNM
    @param dnm pointer to the DNM to be freed
*/
void old_freeDNM(old_dnmPtr dnm);


//ITERATORS

/*! creates an iterator over the document (uses malloc -> has to be free'd later)
    @param dnm the document to be iterated over
  @param level the chunks we want to iterate over (DNM_LEVEL_PARA, DNM_LEVEL_SENTENCE, DNM_LEVEL_WORD)
  @retval a pointer to the iterator
*/
dnmIteratorPtr getDnmIterator(old_dnmPtr dnm, enum dnm_level level);


/*! creates an iterator for the children of the current position of an iterator,
    i.e. over the sentences of a certain paragraph, or over the words of a sentence.
    The new iterator has to be free'd manually as well
    @param it the iterator to which tells us what to iterate over
  @retval the iterator for the children
*/
dnmIteratorPtr getDnmChildrenIterator(dnmIteratorPtr it);

/*! Make an iterator point to the next chunk
    @param it the iterator that shall be incremented
  @retval 0 if the iterator points to the last element already, otherwise 1
*/
int dnmIteratorNext(dnmIteratorPtr it);

/*! Make an iterator point to the previous chunk
    @see dnmIteratorNext
*/
int dnmIteratorPrevious(dnmIteratorPtr it);

/*! Returns the plain text of the chunk an iterator points to
    @param it The iterator
  @retvalue The plain text, ended by \0, which has to be free'd manually
*/
char *getDnmIteratorContent(dnmIteratorPtr it);


//ANNOTATIONS

/*! Checks whether a chunk has a certain annotation.
    Note that a faster way should be implemented,
  if you want to repeatedly check for one annotation.
    @param it A pointer to an iterator
  @param annotation The string representation of the annotation
  @retval 1 if the chunk has the annotation, 0 otherwise
*/
int dnmIteratorHasAnnotation(dnmIteratorPtr it, const char *annotation);

/*! like dnmIteratorHasAnnotation, just for annotations inherited
    (i.e. annotations from the parent tags)
  @see dnmIteratorHasAnnotation
*/
int dnmIteratorHasAnnotationInherited(dnmIteratorPtr it, const char *annotation);

/*! adds an annotation to a chunk.
    Again: A faster way for repeatedly adding one annotation should be implemented.
    @param it An iterator referring to the chunk
  @param annotation The string representation of the annotation
  @param writeIntoDOM If non-zero: The annotation is also written into the DOM
  @param inheritToChildren If non-zero: The annotation is inherited to the child chunks
*/
void dnmIteratorAddAnnotation(dnmIteratorPtr it, const char *annotation, int writeIntoDOM, int inheritToChildren);
