/*! \defgroup para_discr Paragraph Discrimination
    Some tools for experiments concerning paragraph discrimination.
	Paragraph discrimination refers to the idea that in a
	mathematical document, paragraphs tend to have certain functions.
	I.e. they're for example proofs, axioms, definitions, ...
	@file
*/
//#include <json-c/json.h>
#include "jsoninclude.h"
#include <libxml/tree.h>

/*! Collects bags of words, i.e. it counts how often which word
    occurs in which type of paragraph.
	- experimental -
	@param doc The input document with marked up paragraphs
	@retval Returns the results as a JSON object.
*/
json_object* llamapun_para_discr_get_bags (xmlDocPtr doc);
