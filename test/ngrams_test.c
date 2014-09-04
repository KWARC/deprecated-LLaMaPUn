#include <stdio.h>
#include <string.h>
// XML DOM and XPath
#include <libxml/tree.h>
#include <libxml/parser.h>
// JSON
#include <llamapun/json_include.h>
// Ngrams
#include <llamapun/ngrams.h>
#include <llamapun/local_paths.h>

#define FILE_BUFFER_SIZE 2048
char test_xhtml_doc[FILE_BUFFER_SIZE];

int main() {
  sprintf(test_xhtml_doc,"%s/t/documents/1311.6767.xhtml", LLAMAPUN_ROOT_PATH);
  xmlDoc* doc = xmlReadFile(test_xhtml_doc, NULL, 0);
  if (doc == NULL) {
    fprintf(stderr,"Test XHTML document could not be parsed, failing");
    return 1; }
  json_object* response = llamapun_get_ngrams(doc);

  FILE *f = fopen("ngrams.txt", "w");
  fprintf(f, "%s", json_object_to_json_string(response));
  fclose(f); 

  json_object* tmp;
  json_object* tmp2;

  //check the number of occurences of the unigram "fraction"
  #ifndef JSON_OBJECT_OBJECT_GET_EX_DEFINED
  tmp = json_object_object_get(response, "unigrams");
  tmp2 = json_object_object_get(tmp, "fraction");
  #else
  json_object_object_get_ex(response, "unigrams", &tmp);
  json_object_object_get_ex(tmp, "fraction", &tmp2);
  #endif

  if (json_object_get_int(tmp2) != 3) {
    fprintf(stderr, "test ngrams -- wrong result for 'fraction': got %d\n", json_object_get_int(tmp2));
    return 1;
  }

  //check the number of occurences of the bigram "arc length"
  #ifndef JSON_OBJECT_OBJECT_GET_EX_DEFINED
  tmp = json_object_object_get(response, "bigrams");
  tmp2 = json_object_object_get(tmp, "arc length");
  #else
  json_object_object_get_ex(response, "bigrams", &tmp);
  json_object_object_get_ex(tmp, "arc length", &tmp2);
  #endif



  if (json_object_get_int(tmp2) != 2) {
    fprintf(stderr, "test ngrams -- wrong result for 'arc length': got %d\n", json_object_get_int(tmp2));
    return 1;
  }

  json_object_put(response);

  if (doc) xmlFreeDoc(doc);
  xmlCleanupParser();

  return 0;
}
