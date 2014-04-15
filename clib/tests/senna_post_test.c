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
  dom_to_pos_annotations(doc);

  xmlFreeDoc(doc);
  xmlCleanupParser();
  return 0;
}
