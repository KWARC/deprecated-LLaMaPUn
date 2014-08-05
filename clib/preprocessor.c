#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <uthash.h>
#include <libxml/tree.h>


//macro, checking whether memory was allocated successfully
#define CHECK_ALLOC(ptr) {if (ptr==NULL) {fprintf(stderr, "Couldn't allocate memory, exiting\n"); fflush(stderr); exit(1);}}

//resize an array if the index exceeds the old size
#define POSSIBLE_RESIZE(ptr, index, oldsizeptr, newsize, type) \
		{if (index >= *oldsizeptr) {\
			ptr = (type*)realloc(ptr, (newsize)*sizeof(type)); *oldsizeptr=newsize; CHECK_ALLOC(ptr); }}



//=======================================================
//Section: Tag normalization stuff
//=======================================================


struct tmp_preprocessing_data {
	char *plaintext;
	size_t pt_size;
	size_t pt_index;
};

struct hash_item_tag_normalization {
	char *tag_name;
	char *norm_string;
	UT_hash_handle hh;
};

struct hash_item_tag_normalization * tag_norm_handle = NULL;

#define ADD_TAG_NORMALIZATION(name, repl) {tmp = (struct hash_item_tag_normalization *) malloc(sizeof(struct hash_item_tag_normalization)); \
                                        CHECK_ALLOC(tmp); tmp->tag_name = name; tmp->norm_string = repl;\
                                        HASH_ADD_KEYPTR(hh, tag_norm_handle, tmp->tag_name, strlen(tmp->tag_name), tmp);}

void load_tag_repl_hash() {
	if (tag_norm_handle) return;   //it's loaded already
	struct hash_item_tag_normalization * tmp;
	ADD_TAG_NORMALIZATION("math", "[MathFormula]");
	//ADD_TAG_NORMALIZATION("table", "")
	ADD_TAG_NORMALIZATION("script", "");
	ADD_TAG_NORMALIZATION("title", "");
	ADD_TAG_NORMALIZATION("head", "");
	ADD_TAG_NORMALIZATION("bibliography", "");
	ADD_TAG_NORMALIZATION("abstract", "");
	ADD_TAG_NORMALIZATION("toctitle", "");
	ADD_TAG_NORMALIZATION("creator", "");
	ADD_TAG_NORMALIZATION("keywords", "");
	ADD_TAG_NORMALIZATION("classification", "");
	ADD_TAG_NORMALIZATION("acknowledgements", "");
}

#undef ADD_TAG_NORMALIZATION

char *getNormalization(const char *tag_name) {
	struct hash_item_tag_normalization *tmp;
	HASH_FIND_STR(tag_norm_handle, tag_name, tmp);
	if (tmp) return tmp->norm_string;
	else return NULL;   //Don't normalize
}


//=======================================================
//Section: Core preprocessing functionality
//=======================================================

void append_to_plaintext(struct tmp_preprocessing_data *tpd, const char *string) {
	while (*string != '\0') {
		POSSIBLE_RESIZE(tpd->plaintext, tpd->pt_index, &tpd->pt_size,
			tpd->pt_size*2, char);
		tpd->plaintext[tpd->pt_index++] = *string++;
	}
}


void preprocessing_parse(xmlNode *n, struct tmp_preprocessing_data *tpd) {
	xmlNode *node;
	char *normalization;
	char string[32];
	xmlChar *content;
	for (node = n; node!=NULL; node = node->next) {
		if (xmlStrEqual(node->name, BAD_CAST "text")) {
			content = xmlNodeGetContent(node);
			append_to_plaintext(tpd, (char *)content);
			xmlFree(content);
		} else {
			normalization = getNormalization((char *)node->name);
			if (normalization) {
				append_to_plaintext(tpd, normalization);
			} else {
				//store offset
				snprintf(string, sizeof(string), "%ld", tpd->pt_index);
				xmlNewProp(node, BAD_CAST "llamapun_offset_start", BAD_CAST string);

				preprocessing_parse(node->children, tpd);

				snprintf(string, sizeof(string), "%ld", tpd->pt_index);
				xmlNewProp(node, BAD_CAST "llamapun_offset_end", BAD_CAST string);
			}
			
		}
	}
}


void remove_offset_attributes_actual(xmlNode *n) {
	xmlNode *node;
	xmlAttr *attr;
	for (node = n; node!=NULL; node = node->next) {
		for (attr = node->properties; attr != NULL; attr = attr->next) {
			if (xmlStrEqual(attr->name, BAD_CAST "llamapun_offset_start") || 
				xmlStrEqual(attr->name, BAD_CAST "llamapun_offset_end")) {
				if (attr->prev != NULL)
					attr->prev->next = attr->next;
				else
					node->properties = attr->next;
				if (attr->next)
					attr->next->prev = attr->prev;
				xmlFreeProp(attr);
			}
		}
		remove_offset_attributes_actual(node->children);
	}
}


void remove_offset_attributes(xmlDocPtr doc) {
	xmlNode *node = xmlDocGetRootElement(doc);
	remove_offset_attributes_actual(node);
}


char * preprocess(xmlDocPtr doc) {
	load_tag_repl_hash();

	struct tmp_preprocessing_data *tpd = (struct tmp_preprocessing_data *) malloc(sizeof(struct tmp_preprocessing_data));
	CHECK_ALLOC(tpd);

	tpd->plaintext = (char *) malloc(sizeof(char)*2048);
	CHECK_ALLOC(tpd->plaintext);
	tpd->pt_index = 0;
	tpd->pt_size = 2048;


	preprocessing_parse(xmlDocGetRootElement(doc), tpd);


	char *result = realloc(tpd->plaintext, tpd->pt_index+1);
	CHECK_ALLOC(result);
	result[tpd->pt_index] = '\0';
	free(tpd);
	return result;
}




//=======================================================
//Section: Freeing memory and cleaning up
//=======================================================



void preprocessorCleanUp() {
	if (tag_norm_handle) {
		struct hash_item_tag_normalization *current, *tmp;
		HASH_ITER(hh, tag_norm_handle, current, tmp) {
			HASH_DEL(tag_norm_handle, current);
			//strings are not malloc'd
			free(current);
		}
		tag_norm_handle = NULL;
	}
}



//=======================================================
//Section: Fancy features
//=======================================================


xmlNode *getLowestSurroundingNode_actual(xmlNode *n, size_t offset_start, size_t offset_end) {
	xmlNode *node;
	xmlNode *tmpnode;
	xmlAttr *attr;
	xmlChar *tmpxmlstr;
	size_t tmp_int;
	int tags_found_info;
	for (node = n; node!=NULL; node = node->next) {
		tags_found_info = 0;  //reset info
		for (attr = node->properties; attr != NULL; attr = attr->next) {
			if (xmlStrEqual(attr->name, BAD_CAST "llamapun_offset_start")) {
				tags_found_info |= 1;    //found offset_start
				tmpxmlstr = xmlNodeGetContent(attr->children);
				CHECK_ALLOC(tmpxmlstr);
				sscanf((char *)tmpxmlstr, "%lu", &tmp_int);
				if (tmp_int <= offset_start) tags_found_info |= 2;   //offset_start matches
				xmlFree(tmpxmlstr);
			} else if (xmlStrEqual(attr->name, BAD_CAST "llamapun_offset_end")) {
				tags_found_info |= 4;   //found offset_end
				tmpxmlstr = xmlNodeGetContent(attr->children);
				CHECK_ALLOC(tmpxmlstr);
				sscanf((char *)tmpxmlstr, "%lu", &tmp_int);
				if (tmp_int >= offset_end) tags_found_info |= 8;   //offset_end matches
			}
			if ((tags_found_info & 1) && (tags_found_info & 4)) break;  //found all
		}
		//if (!((tags_found_info & 1) && (tags_found_info & 4))) {  //not all offsets are marked
		//	fprintf(stderr, "Warning: Offsets are not marked\n");
		//}
		if ((tags_found_info & 2) && (tags_found_info & 8)) {  //node surrounds given offsets
			tmpnode = getLowestSurroundingNode_actual(node->children, offset_start, offset_end);
			if (tmpnode != NULL) return tmpnode;   //found lower surrounding node
			else return node;
		}
	}
	return NULL;   //haven't found any surrounding node
}

xmlNode *getLowestSurroundingNode(xmlDocPtr doc, size_t offset_start, size_t offset_end) {
	xmlNode *node = xmlDocGetRootElement(doc);
	return getLowestSurroundingNode_actual(node, offset_start, offset_end);
}


long getAttrIntVal(xmlNode *node, xmlChar *attr, long defaultvalue) {
	xmlChar *xmlstr;
	xmlAttr *a;
	long result = defaultvalue;
	for (a = node->properties; a!=NULL; a=a->next) {
		if (xmlStrEqual(a->name, attr)) {  //found attribute
			xmlstr = xmlNodeGetContent(a->children);
			sscanf((char *)xmlstr, "%ld", &result);
		}
	}
	return result;
}

xmlNode *insertNodeAroundOffsets(xmlDocPtr doc, size_t offset_start, size_t offset_end) {
	/* UNFINISHED!!! */
	xmlNode *node = xmlDocGetRootElement(doc);
	xmlNode *surrounder = getLowestSurroundingNode_actual(node, offset_start, offset_end);
	if (surrounder == NULL) return NULL;    //didn't find surrounding node
	
	long surr_start = getAttrIntVal(node, BAD_CAST "llamapun_offset_start", -1);
	long surr_end   = getAttrIntVal(node, BAD_CAST "llamapun_offset_end", -1);
	//return insertNodeAroundOffsets_actual(node, offset_start, offset_end);
	return NULL;
}
