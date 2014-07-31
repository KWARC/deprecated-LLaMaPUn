#include <stdio.h>
#include <string.h>
//#include <json-c/json.h>
#include "../jsoninclude.h"
// XML DOM and XPath
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include "../llamapun_para_discr.h"

const char TEST_XHTML_DOCUMENT[] = "../../t/documents/1311.6767.xhtml";

int main() {

  xmlDoc* doc = xmlReadFile(TEST_XHTML_DOCUMENT, NULL, 0);
  if (doc == NULL) {
    fprintf(stderr,"Test XHTML document could not be parsed, failing");
    return 1; }
  json_object* response = llamapun_para_discr_get_bags(doc);

  FILE *f = fopen("bags.txt", "w");
  fprintf(f, "%s", json_object_to_json_string(response));
  fclose(f); 

  json_object* tmp;

  //check that the word double was found
  #ifndef JSON_OBJECT_OBJECT_GET_EX_DEFINED
  tmp = json_object_object_get(response, "double");
  #else
  json_object_object_get_ex(response, "double", &tmp);
  #endif

  if (tmp == NULL) {
    fprintf(stderr, "test para_discr -- didn't find \"double\" in bag\n");
    return 1;
  }

  json_object_put(response);

  if (doc) xmlFreeDoc(doc);
  xmlCleanupParser();

  return 0;
}
