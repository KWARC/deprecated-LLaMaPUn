/*! \defgroup dnmlib DNMlib
    The Document Narrative Model (DNM) can mainly be used
    for the tokenization (sentence and word splitting)
    during the preprocessing.
      @file
*/

#ifndef DNMLIB_H
#define DNMLIB_H

#include <libxml/tree.h>
#include <uthash.h>

#include <string.h>


/*! normalize tags (eg math tags) in document */
#define DNM_NORMALIZE_TAGS (1 << 0)


/*! string element for uthash */
struct hash_element_string {
  char *string;
  UT_hash_handle hh;
};


//=======================================================
//Section: Types
//=======================================================


struct dnmStruct {
  xmlDocPtr document;
  long parameters;
  char *plaintext;
  size_t size_plaintext;

  unsigned long sentenceCount;   //counter variable for the id attributes
};

/*! pointer to a DNM */
typedef struct dnmStruct * dnmPtr;

struct _dnmRange {
  size_t start;
  size_t end;
};

/*! a range of the plaintext */
typedef struct _dnmRange dnmRange;

struct _dnmRanges {
  dnmRange* range;
  int length;
};

/*! a list of ranges */
typedef struct _dnmRanges dnmRanges;


//=======================================================
//Section: Functions
//=======================================================

/*! creates a DNM

    Memory has to be freed later using freeDNM
  @see freeDNM
  @param doc a pointer to the DOM
  @param parameters the parameters
  @retval a pointer to the new DNM
*/
dnmPtr createDNM(xmlDocPtr doc, long parameters);

/*! frees the DNM
    @param dnm pointer to the DNM to be freed
*/
void freeDNM(dnmPtr dnm);

/*! get the node corresponding to an offset in the plaintext
  @param n the root node
  @param offset the plaintext offset
  @retval a pointer to the lowest with offset annotations around the given offset
*/
xmlNode * get_node_from_offset(xmlNode *n, size_t offset);

//TOKENIZATION
/*! marks a sentence in the DOM (using a span tag)
  
  @param dnm a pointer to the DNM
  @range the offsets of the sentence
  @retval returns 1 if something went wrong (e.g. doesn't fit into XML structure), otherwise 0
*/
int mark_sentence(dnmPtr dnm, dnmRange range);
/*! returns the plaintext of a node
  @param mydnm a pointer to the DNM
  @param mynode a pointer to the node you want to get plaintext from
  @retval a string containing the plaintext (has to be free'd manually)
*/
char* dnm_node_plaintext(dnmPtr mydnm, xmlNodePtr mynode);

#endif