#include <libxml/tree.h>

char * preprocess(xmlDocPtr doc);
void remove_offset_attributes(xmlDocPtr doc);
void preprocessorCleanUp();

xmlNode *getLowestSurroundingNode(xmlDocPtr doc, size_t offset_start, size_t offset_end);
