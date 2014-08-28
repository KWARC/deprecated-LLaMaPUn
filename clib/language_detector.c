#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ftw.h>
#include <sys/stat.h>

#include "third-party/libtextcat-2.2/src/textcat.h"

#include "llamapun_utils.h"
#include "dnmlib.h"

void *my_tc = NULL;

int parse(const char *filename, const struct stat *status, int type) {
  if (type != FTW_F) return 0; //Not a file

  fprintf(stderr, "%s\n", filename);

  xmlDoc *doc = read_document(filename);

  if (doc == NULL) return 0;   //error message printed by read_document

  dnmPtr dnm = createDNM(doc, DNM_SKIP_TAGS);

  if (dnm == NULL) {
    fprintf(stderr, "Couldn't create DNM\n");
    exit(1);
  }

  if (my_tc == NULL) fprintf(stderr, "Fatal\n");

  char *result = textcat_Classify(my_tc, dnm->plaintext, dnm->size_plaintext /*> 5000 ? 5000 : dnm->size_plaintext*/ );

  if (strncmp(result, "[english]", strlen("[english]"))) {  //isn't primarily english
    printf("%s\t%s\n", filename, result);
  }

  freeDNM(dnm);
  xmlFreeDoc(doc);
  return 0;
}


int main(int argc, char *argv[]) {
  my_tc = textcat_Init("../libtextcatconf.txt");
  if (my_tc == NULL) {
    fprintf(stderr, "Fatal: Couldn't load languages (make sure that the paths in ../libtextcatconf.txt are valid)\n");
    exit(1);
  }


  if(argc == 1)
    ftw(".", parse, 1);
  else
    ftw(argv[1], parse, 1);

  textcat_Done(my_tc);
  xmlCleanupParser();
  return 0;
}