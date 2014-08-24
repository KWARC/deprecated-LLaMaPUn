/* CorTeX utility functions and constants */


#ifndef LLAMAPUN_UTILS_H
#define LLAMAPUN_UTILS_H

#include "jsoninclude.h"
#include <libxml/parser.h>
#include <libxml/HTMLparser.h>

char* cortex_stringify_response(json_object* response, size_t *size);

json_object* cortex_response_json(char *annotations, char *message, int status);

xmlDocPtr read_document(const char* filename);

#endif
