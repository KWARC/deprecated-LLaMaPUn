// Standard C includes
#include <stdio.h>
// JSON
#include <json.h>
// XML DOM and XPath
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include "../llamapun_senna_pos.h"

const char TEST_XHTML_DOCUMENT[] = "../../t/documents/1311.6767.xhtml";

int main() {

  xmlDoc* doc = xmlReadFile(TEST_XHTML_DOCUMENT, NULL, 0);
  if (doc == NULL) {
    perror("Test XHTML document could not be parsed, failing");
    return 1; }
  json_object* response = dom_to_pos_annotations(doc);
  /* Clean up libxml objects */
  xmlFreeDoc(doc);
  xmlCleanupParser();

  int words_total = 0;
  char *key; struct json_object *val; struct lh_entry *entry;
  for(entry = json_object_get_object(response)->head; (entry ? (key = (char*)entry->k, val = (struct json_object*)entry->v, entry) : 0); entry = entry->next) {
    words_total++;
  }
  if (words_total != 6189) {
    printf("POS tagged words -- count mismatch, got %d \n",words_total);
    return 1;  }
//  char* rdfxml_pos = pos_labels_to_rdfxml(response);
//  printf("%s",rdfxml_pos);


  return 0;
}
