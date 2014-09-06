#include "dnmlib.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>


//macro, checking whether memory was allocated successfully
#define CHECK_ALLOC(ptr) {if (ptr==NULL) {fprintf(stderr, "Couldn't allocate memory, exiting\n"); fflush(stderr); exit(1);}}

//resize an array if the index exceeds the old size
#define POSSIBLE_RESIZE(ptr, index, oldsizeptr, newsize, type) \
    {if (index >= *oldsizeptr) {\
      ptr = (type*)realloc(ptr, (newsize)*sizeof(type)); *oldsizeptr=newsize; CHECK_ALLOC(ptr); }}



//=======================================================
//Section: Tokenization
//=======================================================


dnmRange get_range_of_node(xmlNode *node) {
  xmlAttr *attr;
  dnmRange answer;
  answer.start = 0;
  answer.end = 0;

  xmlChar *value;
  for (attr = node->properties; attr != NULL; attr = attr->next) {
    if (xmlStrEqual(attr->name, BAD_CAST "dnm_from")) {
      value = xmlNodeGetContent(attr->children);
      sscanf((char *)value, "%ld", &(answer.start));
      xmlFree(value);
    }
    else if (xmlStrEqual(attr->name, BAD_CAST "dnm_to")) {
      value = xmlNodeGetContent(attr->children);
      sscanf((char *)value, "%ld", &(answer.end));
      xmlFree(value);
    }
  }
  return answer;
}


xmlNode * get_lowest_surrounding_node(xmlNode *n, dnmRange range) {
  xmlNode *node;
  dnmRange r;
  xmlNode *tmp;
  for (node=n; node!=NULL; node=node->next) {
    r = get_range_of_node(node);
    if (r.start <= range.start && r.end >= range.end) {
      tmp = get_lowest_surrounding_node(node->children, range);
      if (tmp != NULL) return tmp;
      else return node;
    }
  }
  return NULL;   //didn't find anything
}


xmlNode * get_node_from_offset(xmlNode *n, size_t offset) {
  xmlNode *node;
  dnmRange r;
  xmlNode *tmp;
  for (node=n; node!=NULL; node=node->next) {
    r = get_range_of_node(node);
    if (r.start <= offset && r.end > offset) {
      tmp = get_node_from_offset(node->children, offset);
      if (tmp != NULL) return tmp;
      else return node;
    }
  }
  return NULL;   //didn't find anything
}


char* dnm_node_plaintext(dnmPtr mydnm, xmlNodePtr mynode) {
  dnmRange range = get_range_of_node(mynode);
  if (range.end < range.start) {
    fprintf(stderr, "ERROR (dnmlib): llamapun range start larger than end\n");
    return NULL;
  } else if (range.end == 0) {   //no offset annotations
    return NULL;
  } else if (range.end > mydnm->size_plaintext) {
    fprintf(stderr, "ERROR (dnmlib): llamapun offset out of range\n");
    return NULL;
  }

  char tmp = mydnm->plaintext[range.end+1];
  mydnm->plaintext[range.end+1] = '\0';
  char *answer = strdup(mydnm->plaintext+range.start);
  if (range.end > range.start) CHECK_ALLOC(answer);
  mydnm->plaintext[range.end+1] = tmp;
  return answer;
}

int is_llamapun_text_wrap(xmlNode * n) {
  if (!xmlStrEqual(n->name, BAD_CAST "span")) return 0;
  xmlAttr *attr;
  xmlChar *xmlstr;
  int retval;
  for (attr = n->properties; attr != NULL; attr = attr->next) {
    if (xmlStrEqual(attr->name, BAD_CAST "class")) {
      xmlstr = xmlNodeGetContent(attr->children);
      retval = xmlStrEqual(xmlstr, BAD_CAST "dnm_wrapper");
      xmlFree(xmlstr);
      return retval;
    }
  }
  return 0;   //isn't marked with attribute span="dnm_wrapper"
}

int mark_sentence(dnmPtr dnm, dnmRange range) {
  if (range.end <= range.start) {
    //empty sentence
    return 1;
  }
  xmlNode * parent = get_lowest_surrounding_node(dnm->root, range);
  //note: we know that parent has children, because it's a node with offset annotations

  xmlNode *start = NULL;
  xmlNode *end = NULL;
  xmlNode *old_starttextnode = NULL;
  xmlNode *old_endtextnode = NULL;
  xmlNode *tmpnode;
  xmlNode *tmpnode2;
  xmlNode *tmpnode3;
  xmlChar *tmpxmlstr;
  xmlChar tmpxmlchar;
  char string[64];

  xmlNode *it = parent->children;    //iterator over children
  dnmRange tmprange;
  //find start node
  while (start == NULL && it != NULL) {
    tmprange = get_range_of_node(it);
    if (tmprange.end > range.start) {  //range.start in range of it
      if (range.start == tmprange.start) {
        start = it;
      } else if (is_llamapun_text_wrap(it)) {
        //in this case we are allowed to and forced to split it
        old_starttextnode = it;
        tmpxmlstr = xmlNodeGetContent(it->children);
        tmpxmlchar = tmpxmlstr[range.start - tmprange.start];

        //create FIRST node
        tmpxmlstr[range.start - tmprange.start] = '\0';
        tmpnode = xmlNewNode(NULL, BAD_CAST "span");
        tmpnode3 = xmlNewText(tmpxmlstr);
        xmlAddChild(tmpnode, tmpnode3);
        xmlNewProp(tmpnode, BAD_CAST "class", BAD_CAST "dnm_wrapper");
        snprintf(string, sizeof(string), "%ld", tmprange.start);
        xmlNewProp(tmpnode, BAD_CAST "dnm_from", BAD_CAST string);
        snprintf(string, sizeof(string), "%ld", range.start);
        xmlNewProp(tmpnode, BAD_CAST "dnm_to", BAD_CAST string);
        xmlAddNextSibling(old_starttextnode, tmpnode);

        //create second node
        tmpxmlstr[range.start - tmprange.start] = tmpxmlchar;
        tmpnode2 = xmlNewNode(NULL, BAD_CAST "span");
        tmpnode3 = xmlNewText(tmpxmlstr + (range.start-tmprange.start));
        xmlAddChild(tmpnode2, tmpnode3);
        xmlNewProp(tmpnode2, BAD_CAST "class", BAD_CAST "dnm_wrapper");
        snprintf(string, sizeof(string), "%ld", range.start);
        xmlNewProp(tmpnode2, BAD_CAST "dnm_from", BAD_CAST string);
        snprintf(string, sizeof(string), "%ld", tmprange.end);
        xmlNewProp(tmpnode2, BAD_CAST "dnm_to", BAD_CAST string);
        xmlAddNextSibling(tmpnode, tmpnode2);

        start = tmpnode2;
        it = start;
        xmlUnlinkNode(old_starttextnode);

        xmlFree(tmpxmlstr);
      } else {     //hits inside unsplittable tag or wouldn't be well-formatted xhtml
        return 1;
      }
    } else {
      it = it->next;
    }
  }

  if (start == NULL) return 1;   //didn't find start node in range

  //now we have the start node, so we start looking for the end node
  while (end == NULL && it != NULL) {
    tmprange = get_range_of_node(it);
    if (tmprange.end >= range.end) {   //we're in the range
      if (tmprange.end == range.end) {   //the ends match, so we don't need to split
        end = it;
      } else if (is_llamapun_text_wrap(it)) {
        //we need to split
        old_endtextnode = it;
        tmpxmlstr = xmlNodeGetContent(it->children);
//        fprintf(stderr, "%s    %ld   %ld   -  off %ld\n", (char *) tmpxmlstr, range.end, tmprange.start, range.end-tmprange.start);
        tmpxmlchar = tmpxmlstr[range.end - tmprange.start];

        //create FIRST node
        tmpxmlstr[range.end - tmprange.start] = '\0';
        tmpnode = xmlNewNode(NULL, BAD_CAST "span");
        tmpnode3 = xmlNewText(tmpxmlstr);
        xmlAddChild(tmpnode, tmpnode3);
        xmlNewProp(tmpnode, BAD_CAST "class", BAD_CAST "dnm_wrapper");
        snprintf(string, sizeof(string), "%ld", tmprange.start);
        xmlNewProp(tmpnode, BAD_CAST "dnm_from", BAD_CAST string);
        snprintf(string, sizeof(string), "%ld", range.end);
        xmlNewProp(tmpnode, BAD_CAST "dnm_to", BAD_CAST string);
        xmlAddNextSibling(old_endtextnode, tmpnode);

        //create second node
        tmpxmlstr[range.end - tmprange.start] = tmpxmlchar;
        tmpnode2 = xmlNewNode(NULL, BAD_CAST "span");
        tmpnode3 = xmlNewText(tmpxmlstr + (range.end-tmprange.start));
        xmlAddChild(tmpnode2, tmpnode3);
        xmlNewProp(tmpnode2, BAD_CAST "class", BAD_CAST "dnm_wrapper");
        snprintf(string, sizeof(string), "%ld", range.end);
        xmlNewProp(tmpnode2, BAD_CAST "dnm_from", BAD_CAST string);
        snprintf(string, sizeof(string), "%ld", tmprange.end);
        xmlNewProp(tmpnode2, BAD_CAST "dnm_to", BAD_CAST string);
        xmlAddNextSibling(tmpnode, tmpnode2);

        end = tmpnode;
        //it = tmpnode2;
        xmlUnlinkNode(old_endtextnode);

        xmlFree(tmpxmlstr);
      } else {   //we hit an unsplittable tag around the end of the range
        return 1;
      }
    } else {
      it = it->next;
    }
  }

  if (end == NULL) {
    if (old_starttextnode != NULL) {
      //undo the start node splitting
      xmlAddNextSibling(start, old_starttextnode);
      tmpnode = start->prev;
      xmlUnlinkNode(start->prev);
      xmlFreeNode(tmpnode);
      xmlUnlinkNode(start);
      xmlFreeNode(start);
      return 1;
    }
    return 1;
  }

  xmlFreeNode(old_starttextnode);
  xmlFreeNode(old_endtextnode);

  //put nodes into sentence tag
  tmpnode = xmlNewNode(NULL, BAD_CAST "span");
  xmlAddNextSibling(end, tmpnode);
  it = start;
  while (it != tmpnode) {
    tmpnode2 = it->next;
    xmlUnlinkNode(it);
    xmlAddChild(tmpnode, it);
    it = tmpnode2;
  }

  xmlNewProp(tmpnode, BAD_CAST "class", BAD_CAST "ltx_sentence");    //done like this by old standard - rename to dnm_sentence??
  snprintf(string, sizeof(string), "sentence.%ld", dnm->sentenceCount++);
  xmlNewProp(tmpnode, BAD_CAST "id", BAD_CAST string);
  snprintf(string, sizeof(string), "%ld", range.start);
  xmlNewProp(tmpnode, BAD_CAST "dnm_from", BAD_CAST string);
  snprintf(string, sizeof(string), "%ld", range.end);
  xmlNewProp(tmpnode, BAD_CAST "dnm_end", BAD_CAST string);

  return 0;   //everything went well
}


//=======================================================
//Section: Creating DNM
//=======================================================


//for the creation of the DNM, we need to keep some things in mind:
struct tmpParseData {
  size_t plaintext_allocated;
  size_t plaintext_index;
};

void wrap_text_into_spans(xmlNode *node) {
  /* puts text which has sibling nodes into a span (simplifies the offset stuff a lot) */
  if (xmlStrEqual(node->name, BAD_CAST "text")) {
    if (node->next != NULL || node->prev != NULL) {  //if node has siblings
      xmlNode *newNode  = xmlNewNode(NULL, BAD_CAST "span");
      xmlAddNextSibling(node, newNode);
      xmlUnlinkNode(node);
      xmlAddChild(newNode, node);
      xmlNewProp(newNode, BAD_CAST "class", BAD_CAST "dnm_wrapper");
      node = newNode;  //continue from the span
    }
  }

  xmlNode *n;
  for (n=node->children; n!=NULL; n=n->next) {
    wrap_text_into_spans(n);
  }
}


void copy_into_plaintext(const char *string, dnmPtr dnm, struct tmpParseData *dcs) {
  /* appends string to the plaintext, stored in dnm */
  while (*string != '\0') {
    POSSIBLE_RESIZE(dnm->plaintext, dcs->plaintext_index, &dcs->plaintext_allocated,
      dcs->plaintext_allocated*2, char);
    dnm->plaintext[dcs->plaintext_index++] = *string++;
  }
}

int has_class_value(xmlNode* node, char *val) {
  xmlChar* tmpxmlstr;
  xmlAttr* attr;
  for (attr=node->properties; attr!=NULL; attr=attr->next) {
    if (xmlStrEqual(attr->name, BAD_CAST "class")) {
      tmpxmlstr = xmlNodeGetContent(attr->children);
      int retval = xmlStrEqual(tmpxmlstr, BAD_CAST val);
      xmlFree(tmpxmlstr);
      return retval;
    }
  }
  return 0;   //doesn't have attribute => doesn't have value (somehow)
}


void parse_dom_into_dnm(xmlNode *node, dnmPtr dnm, struct tmpParseData *dcs, long parameters) {
  /* the core function which (recursively) parses the DOM into the DNM */

  //declaring a lot of variables required later on...
  xmlChar *tmpxmlstr;
  char offsetstring[32];

  //write start offset into DOM
  if (!(parameters & DNM_NO_OFFSETS)) {
    snprintf(offsetstring, sizeof(offsetstring), "%ld", dcs->plaintext_index);
    xmlNewProp(node, BAD_CAST "dnm_from", BAD_CAST offsetstring);
  }

  //DEAL WITH TAG
  //possibly normalize math tags
  if ((parameters&DNM_NORMALIZE_TAGS) && xmlStrEqual(node->name, BAD_CAST "math")) {
    copy_into_plaintext(" MathFormula ", dnm, dcs);
  } else if ((parameters&DNM_SKIP_TAGS) && xmlStrEqual(node->name, BAD_CAST "math")) {
    //just don't do anything
  }
  //possibly normalize cite tags
  else if ((parameters&DNM_NORMALIZE_TAGS) && xmlStrEqual(node->name, BAD_CAST "cite")) {
    copy_into_plaintext(" CiteExpression ", dnm, dcs);
  } else if ((parameters&DNM_SKIP_TAGS) && xmlStrEqual(node->name, BAD_CAST "cite")) {
    //just don't do anything
  }
  //possibly normalize tables
  else if ((parameters&DNM_NORMALIZE_TAGS) && xmlStrEqual(node->name, BAD_CAST "table")) {
    copy_into_plaintext(" TableStructure ", dnm, dcs);
  } else if ((parameters&DNM_SKIP_TAGS) && xmlStrEqual(node->name, BAD_CAST "table")) {
    //don't do anything
  }
  //skip head
  else if (xmlStrEqual(node->name, BAD_CAST "head")) {
    //don't to anything
  }
  //copy contents of text nodes into plaintext
  else if (xmlStrEqual(node->name, BAD_CAST "text")) {
    tmpxmlstr = xmlNodeGetContent(node);
    CHECK_ALLOC(tmpxmlstr);
    copy_into_plaintext((char*)tmpxmlstr, dnm, dcs);
    xmlFree(tmpxmlstr);
  }
  //possibly ignore note
  else if ((parameters&DNM_IGNORE_LATEX_NOTES) && (has_class_value(node, "ltx_note_mark") || has_class_value(node, "ltx_note_outer"))) {
    //don't do anything
  }
  //otherwise, parse children recursively
  else {
    //iterate over nodes
    xmlNode *n;

    for (n = node->children; n!=NULL; n = n->next) {
      parse_dom_into_dnm(n, dnm, dcs, parameters); }
  }

  //write end offset into DOM
  if (!(parameters & DNM_NO_OFFSETS)) {
    snprintf(offsetstring, sizeof(offsetstring), "%ld", dcs->plaintext_index);
    xmlNewProp(node, BAD_CAST "dnm_to", BAD_CAST offsetstring);
  }
}

dnmPtr create_DNM(xmlNode *root, long parameters) {
  /* Creates a DNM and returns a pointer to it.
     The memory has to be free'd later by calling free_DNM */

  //check arguments
  if (root==NULL) {
    fprintf(stderr, "dnmlib - Didn't get an root node - parse error??\n");
    return NULL;
  }

  //----------------INITIALIZE----------------
  //dcs (data required only during parsing)
  struct tmpParseData *dcs = (struct tmpParseData*)malloc(sizeof(struct tmpParseData));
  CHECK_ALLOC(dcs);

  //dnm
  dnmPtr dnm = (dnmPtr)malloc(sizeof(struct dnmStruct));
  CHECK_ALLOC(dnm);
  dnm->parameters = parameters;

  dnm->root = root;

  //plaintext
  dnm->plaintext = (char*)malloc(sizeof(char)*4096);
  CHECK_ALLOC(dnm->plaintext);

  dcs->plaintext_allocated = 4096;
  dcs->plaintext_index = 0;

  //ids
  dnm->sentenceCount = 1;   //in the old preprocessor, the first sentence had id="sentence.1"

  //do the actual parsing
  if (!(parameters & DNM_NO_OFFSETS)) {
    wrap_text_into_spans(root);   //only needed for writing offsets into DOM
  }
  parse_dom_into_dnm(root, dnm, dcs, parameters);


  //----------------CLEAN UP----------------

  //end plaintext with \0 and truncate array
  POSSIBLE_RESIZE(dnm->plaintext, dcs->plaintext_index, &dcs->plaintext_allocated,
      dcs->plaintext_allocated+1, char);
  dnm->plaintext[dcs->plaintext_index++] = '\0';
  dnm->size_plaintext = dcs->plaintext_index;
  dnm->plaintext = realloc(dnm->plaintext, dcs->plaintext_index * sizeof(char));
  if (dnm->size_plaintext) CHECK_ALLOC(dnm->plaintext);

  //we don't need the dcs anymore
  free(dcs);

  return dnm;
}



//=======================================================
//Section: Freeing DNM
//=======================================================

void free_DNM(dnmPtr dnm) {
  /* frees the dnm */

  //free plaintext
  free(dnm->plaintext);
  //free level lists
  free(dnm);
}

//=======================================================
//Section: Conversion Utilities
//=======================================================
char* plain_range_to_string(char* text, dnmRange range) {
  unsigned int start = range.start;
  unsigned int end = range.end;
  unsigned int length = end-start+1;
  char* string = malloc(sizeof(char) * (end-start+2));
  memcpy( string, &text[start], length);
  string[length] = '\0';
  return string;
}

char* dnm_range_to_string(dnmPtr mydnm, dnmRange range) {
  return plain_range_to_string(mydnm->plaintext, range);
}
