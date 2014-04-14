// JSON
#include <json-c/json.h>
// XML DOM and XPath
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

json_object* dom_to_pos_annotations (xmlDocPtr doc);
