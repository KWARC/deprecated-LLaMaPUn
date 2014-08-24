#include <stdio.h>
#include "string.h"
#include "../jsoninclude.h"
#include "../dnmlib.h"
#include "../unicode_normalizer.h"
#include "../llamapun_utils.h"

const char TEST_XHTML_DOCUMENT[] = "../../t/documents/1311.0066.xhtml";


int is_ASCII(unsigned char *string) {
  /* returns 1 if string contains only ascii characters, otherwise 0 */
  while (*string) {
    if (*string++ > 127) {
      return 0;
    }
  }
  return 1;   //didn't find any non-ascii character
}


int main(void) {
  //xmlDoc* doc = xmlReadFile(TEST_XHTML_DOCUMENT, NULL, 0);
  xmlDoc *doc = read_document(TEST_XHTML_DOCUMENT);
  if (doc == NULL) {
    fprintf(stderr,"Test XHTML document could not be parsed, failing\n");
    return 1; }
  unicode_normalize_dom(doc);
  dnmPtr dnm = createDNM(doc, DNM_NORMALIZE_TAGS);
  if (strlen(dnm->plaintext) < 100) {   //something could have gone wrong...
    fprintf(stderr, "The plaintext seems to be way too short\n");
    return 1;
  }
  if (!is_ASCII((unsigned char *)dnm->plaintext)) {
    fprintf(stderr, "The unicode-normalized document contains non-ascii characters\n");
    return 1;
  }
  freeDNM(dnm);
  xmlFreeDoc(doc);
  xmlCleanupParser();
  return 0;
}