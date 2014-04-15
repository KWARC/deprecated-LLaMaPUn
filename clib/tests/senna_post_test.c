// Standard C includes
#include <stdio.h>
// XML DOM and XPath
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include "../llamapun_senna_pos.h"

const char TEST_XHTML_DOCUMENT[] = "../../t/documents/entry216.xhtml";

int main() {
  FILE* input_handle = fopen(TEST_XHTML_DOCUMENT,"r");
  if (input_handle == NULL) {
    perror("Test XHTML document can't be opened, failing");
    return 1; }



  fclose(input_handle);
  return 0;
}
