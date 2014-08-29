/* CorTeX utility functions and constants */


#ifndef LLAMAPUN_UTILS_H
#define LLAMAPUN_UTILS_H

#include "jsoninclude.h"
#include <libxml/parser.h>
#include <libxml/HTMLparser.h>

char* cortex_stringify_response(json_object* response, size_t *size);

json_object* cortex_response_json(char *annotations, char *message, int status);

/* reads a document and returns the DOM

  Tries both, interpreting the files as XHTML, and interpreting it as HTML5
  @param filename the file name
  @retval a pointer to the DOM (NULL if reading/parsing wasn't successful)
*/
xmlDocPtr read_document(const char* filename);

#endif
