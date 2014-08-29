#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ftw.h>
#include <sys/stat.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include "llamapun/utils.h"
#include "llamapun/dnmlib.h"
#include "llamapun/unicode_normalizer.h"
int file_counter=0;
xmlChar *paragraph_xpath = (xmlChar*) "//*[local-name()='div' and @class='ltx_para']";

int process_file(const char *filename, const struct stat *status, int type) {
  if (file_counter > 100) { return 0; } // Limit for development
  if (type != FTW_F) return 0; //Not a file
  xmlDoc *doc = read_document(filename);
  if (doc == NULL) return 0;   //error message printed by read_document
  /* 1. Normalize Unicode */
  unicode_normalize_dom(doc);
  /* 2. Select paragraphs */
  xmlXPathContextPtr xpath_context = xmlXPathNewContext(doc);
  if(xpath_context == NULL) {
    fprintf(stderr,"Error: unable to create new XPath context for %s\n",filename);
    xmlFreeDoc(doc);
    exit(1); }
  xmlXPathObjectPtr paragraphs_result = xmlXPathEvalExpression(paragraph_xpath,xpath_context);
  if (paragraphs_result == NULL) { // Nothing to do if there's no math in the document
    return 0; }
  xmlNodeSetPtr paragraph_nodeset = paragraphs_result->nodesetval;
  int para_index;
  fprintf(stderr, "Found %d paragraphs\n", paragraph_nodeset->nodeNr);
  for (para_index=0; para_index < paragraph_nodeset->nodeNr; para_index++) {
    xmlNodePtr paragraph_node = paragraph_nodeset->nodeTab[para_index];
    // TODO: Continue here once createDNM works on libxml Nodes
//    dnmPtr paragraph_dnm = createDNM(paragraph_node, DNM_SKIP_TAGS);
//    if (paragraph_dnm == NULL) {
//      fprintf(stderr, "Couldn't create DNM for paragraph %d in document %s\n",para_index, filename);
//      exit(1);
//    }
//    freeDNM(paragraph_dnm);
    // We have the paragraph text, tokenize and obtain words:
  }
  free(paragraphs_result->nodesetval->nodeTab);
  free(paragraphs_result->nodesetval);
  xmlFree(paragraphs_result);
  xmlFreeDoc(doc);
  fprintf(stderr,"DNM %d\n",file_counter++);
  return 0;
}

int main(int argc, char *argv[]) {
  char *source_directory, *destination_directory;
  switch (argc) {
    case 0:
    case 1:
      source_directory = destination_directory = ".";
      break;
    case 2:
      source_directory = argv[1];
      destination_directory = ".";
      break;
    case 3:
      source_directory = argv[1];
      destination_directory = argv[2];
      break;
    default:
      fprintf(stderr, "Too many arguments: %d\nUsage: ./gen_TF_IDF source_dir/ destination_dir/\n",argc);
      exit(1);
  }

  ftw(source_directory, process_file, 1);
  xmlCleanupParser();
  return 0;
}
