/*! \defgroup ngrams The ngram library
    Consists of a single function for finding ngrams so far.
    @file
*/
    
#ifndef LLAMAPUN_NGRAMS_H
#define LLAMAPUN_NGRAMS_H

//#include <json-c/json.h>
#include "jsoninclude.h"

// XML DOM and XPath
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>


/*! creates and returns statistics of the unigrams, bigrams, and trigrams
    found in a document.
    @param doc the DOM
    @retval returns the counts as a JSON object
*/
json_object* llamapun_get_ngrams (xmlDocPtr doc);

#endif