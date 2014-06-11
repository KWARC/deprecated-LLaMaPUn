#include <stdio.h>
#include <string.h>
#include <json.h>
// XML DOM and XPath
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include "../llamapun_ngrams.h"

const char TEST_XHTML_DOCUMENT[] = "../../t/documents/1311.6767.xhtml";

int main() {

  xmlDoc* doc = xmlReadFile(TEST_XHTML_DOCUMENT, NULL, 0);
  if (doc == NULL) {
    fprintf(stderr,"Test XHTML document could not be parsed, failing");
    return 1; }
  json_object* response = get_ngrams(doc);

  /* FILE *f = fopen("output.txt", "w");
  fprintf(f, "%s", json_object_to_json_string(response));
  fclose(f); */

  /* Clean up libxml objects */
  xmlFreeDoc(doc);
  xmlCleanupParser();

//  //check the number of the unigram "fraction"
//  /*json_object_object_foreach(response, key, value) {
//    if (!strcmp(key, "unigrams")) {
//      json_object_object_foreach(value, key2, value2) {
//        if (!strcmp(key, "fraction")) {
//          printf("%d occurences\n", value2);
//          break;
//        }
//      }
//      break;
//    }
//  }*/
//
  json_object* tmp;



  //check the number of occurences of the unigram "fraction"

  tmp = json_object_object_get(response, "unigrams");
  tmp = json_object_object_get(tmp, "fraction");
  if (json_object_get_int(tmp) != 3) {
    fprintf(stderr, "test ngrams -- wrong result for 'fraction': got %d\n", json_object_get_int(tmp));
    return 1;
  }

  //check the number of occurences of the bigram "arc length"
  tmp = json_object_object_get(response, "bigrams");
  tmp = json_object_object_get(tmp, "arc length");
  if (json_object_get_int(tmp) != 2) {
    fprintf(stderr, "test ngrams -- wrong result for 'arc length': got %d\n", json_object_get_int(tmp));
    return 1;
  }

  return 0;
}
