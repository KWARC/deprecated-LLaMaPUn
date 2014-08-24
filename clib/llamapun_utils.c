// Some everyday C includes
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "llamapun_utils.h"


char* cortex_stringify_response(json_object* response, size_t *size) {
  const char* string_response = json_object_to_json_string(response);
  *size = strlen(string_response);
  return (char *)string_response; }

json_object* cortex_response_json(char *annotations, char *message, int status) {
  json_object *response = json_object_new_object();
  json_object *json_log = json_object_new_string(message);
  json_object *json_status = json_object_new_int(status);
  json_object *json_annotations = json_object_new_string(annotations);

  json_object_object_add(response,"annotations", json_annotations);
  json_object_object_add(response,"log", json_log);
  json_object_object_add(response,"status", json_status);
  return response; }


xmlDocPtr read_document(const char* filename) {
  //read file
  char *content;
  size_t size;
  FILE *file = fopen(filename, "rb");
  if (file == NULL) {
    fprintf(stderr, "Couldn't open %s\n", filename);
    return NULL;
  }
  fseek(file, 0, SEEK_END);
  size = ftell(file);
  fseek(file, 0, SEEK_SET);
  content = (char *) malloc(sizeof(char) * (size + 1));
  fread(content, sizeof(char), size, file);
  fclose(file);
  content[size] = '\0';
  
/* the following is copied and adapted from
   https://github.com/KWARC/mws/blob/master/src/crawler/parser/XmlParser.cpp#L65 */
  xmlDocPtr doc;
  // Try as XHTML
  doc = xmlReadMemory(content, size, /* URL = */ "", "UTF-8",
                            XML_PARSE_NOWARNING | XML_PARSE_NOERROR);
  if (doc != NULL) {
    free(content);
    return doc;
  }
  // Try as HTML
  htmlParserCtxtPtr htmlParserCtxt = htmlNewParserCtxt();
  doc = htmlCtxtReadMemory(
          htmlParserCtxt, content, size, /* URL = */ "", "UTF-8",
          HTML_PARSE_RECOVER | HTML_PARSE_NOWARNING | HTML_PARSE_NOERROR);
  htmlFreeParserCtxt(htmlParserCtxt);
  free(content);
  if (doc != NULL) {
    return doc;
  }

  //if nothing works
  fprintf(stderr, "Couldn't parse %s\n", filename);
  return NULL;
}