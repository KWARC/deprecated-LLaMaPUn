/*! \defgroup unicode_normalizer Unicode Normalizer
    provides tools for normalizing unicode to ascii.
	It uses libiconv.
	@file
*/
#ifndef UNICODE_NORMALIZER_H
#define UNICODE_NORMALIZER_H
#include <libxml/tree.h>

/*! Creates a normalized copy of a string
    @param input the input string
	@param output a pointer to the (normalized) output
*/
void normalize_unicode(char *input, char **output);
void unicode_normalize_dom(xmlDocPtr doc);

#endif
