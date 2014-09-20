#ifndef DOCUMENT_LOADER_H
#define DOCUMENT_LOADER_H

#include <libxml/parser.h>
#include <libxml/HTMLparser.h>


#define DOC_NORMALIZE_UNICODE 1 << 0
#define DOC_CHECK_LANGUAGE 1 << 1
#define DOC_NORMALIZE_NUMBERS 1 << 2

void init_document_loader();
void close_document_loader();

void traverse_docs_in_dir(char *dir, void (*function)(xmlDocPtr, const char *), long parameters, FILE *logfile);

#endif