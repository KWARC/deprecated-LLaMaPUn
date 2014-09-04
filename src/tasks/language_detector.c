#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ftw.h>
#include <sys/stat.h>

#include <textcat.h>

#include "llamapun/language_detection.h"
#include "llamapun/utils.h"
#include "llamapun/dnmlib.h"

void *my_tc = NULL;

int parse(const char *filename, const struct stat *status, int type) {
  if (type != FTW_F) return 0; //Not a file

  fprintf(stderr, "%s\n", filename);

  xmlDoc *doc = read_document(filename);

  if (doc == NULL) return 0;   //error message printed by read_document

  dnmPtr dnm = create_DNM(xmlDocGetRootElement(doc), DNM_SKIP_TAGS);

  if (dnm == NULL) {
    fprintf(stderr, "Couldn't create DNM\n");
    exit(1);
  }

  if (!text_is_english(my_tc, dnm->plaintext, dnm->size_plaintext)) {  //isn't primarily english
    printf("%s\n", filename);
  }

  free_DNM(dnm);
  xmlFreeDoc(doc);
  return 0;
}


int main(int argc, char *argv[]) {
  my_tc = llamapun_textcat_Init();
  if (my_tc == NULL) {
    fprintf(stderr, "Fatal: Couldn't load textcat handle\n");
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
