/*! \defgroup dnmlib DNMlib
    The Document Narrative Model (DNM) can mainly be used
    for the tokenization (sentence and word splitting)
    during the preprocessing.
	@file
*/

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


struct dnm_struct {
	xmlDocPtr document;
	long parameters;

	char *plaintext;

	size_t size_plaintext;
};

typedef struct dnm_struct * dnmPtr;



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


//TOKENIZATION
void markSentences(dnmPtr dnm, size_t *offset_starts, size_t *offset_ends, size_t n);
