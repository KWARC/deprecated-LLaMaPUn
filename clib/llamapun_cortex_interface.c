#include "llamapun_cortex_interface.h"
#include <string.h>
#include <libxml/parser.h>


json_object* llamapun_get_ngrams(json_object* workload) {
	json_object * doc = json_object_object_get(workload, "document");
	char *xmlstring = json_object_get_string(doc);
	json_object * answer = get_ngrams(xmlParseMemory(xmlstring, strlen(xmlstring)));
	//free(xmlstring);
	json_object_put(doc);
	return answer;
}