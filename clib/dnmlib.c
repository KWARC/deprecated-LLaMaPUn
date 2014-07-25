#include "dnmlib.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>


//Let's do it the "clean" way
#define CHECK_ALLOC(ptr) {if (ptr==NULL) {fprintf(stderr, "Couldn't allocate memory, exiting\n"); fflush(stderr); exit(1);}}

#define POSSIBLE_RESIZE(ptr, index, oldsizeptr, newsize, type) \
		{if (index >= *oldsizeptr) {\
			ptr = (type*)realloc(ptr, (newsize)*sizeof(type)); *oldsizeptr=newsize; CHECK_ALLOC(ptr); }}

char * getAnnotationPtr(dnmPtr dnm, const char *annotation, int create);

//=======================================================
//Section: Iterators
//=======================================================

dnmIteratorPtr getDnmIterator(dnmPtr dnm, enum dnm_level level) {
	struct dnm_iterator * it = (struct dnm_iterator *)malloc(sizeof(struct dnm_iterator));
	CHECK_ALLOC(it);
	//set properties
	it->dnm = dnm;
	it->level = level;
	it->pos = 0;
	it->start = 0;
	//level dependent properties
	switch (it->level) {
		case DNM_LEVEL_PARA:
			it->end = it->dnm->size_para_level;
			break;
		case DNM_LEVEL_SENTENCE:
			it->end = it->dnm->size_sent_level;
			break;
		case DNM_LEVEL_WORD:
			it->end = it->dnm->size_word_level;
			break;
		case DNM_LEVEL_NONE:     //avoids compiler warnings
			it->end = 0;
			break;
	}
	return it;
}

dnmIteratorPtr getDnmChildrenIterator(dnmIteratorPtr it) {
	struct dnm_iterator * new_it = (struct dnm_iterator *)malloc(sizeof(struct dnm_iterator));
	CHECK_ALLOC(new_it);

	struct dnm_chunk *chunk;
	switch (it->level) {
		case DNM_LEVEL_PARA:
			chunk = (it->dnm->para_level)+(it->pos);
			new_it->level = DNM_LEVEL_SENTENCE;
			break;
		case DNM_LEVEL_SENTENCE:
			chunk = (it->dnm->sent_level)+(it->pos);
			new_it->level = DNM_LEVEL_WORD;
			break;
		case DNM_LEVEL_WORD:   //level has no children
		case DNM_LEVEL_NONE:   //level has no children
			free(new_it);
			return NULL;
	}

	new_it->dnm = it->dnm;
	new_it->pos = chunk->offset_children_start;
	new_it->start = chunk->offset_children_start;
	new_it->end = chunk->offset_children_end;

	if (new_it->start >= new_it->end) {  //no children
		free(new_it);
		return NULL;
	}

	return new_it;
}

int dnmIteratorNext(dnmIteratorPtr it) {
	size_t level_size;
	switch (it->level) {
		case DNM_LEVEL_PARA:
			level_size = it->dnm->size_para_level;
			break;
		case DNM_LEVEL_SENTENCE:
			level_size = it->dnm->size_sent_level;
			break;
		case DNM_LEVEL_WORD:
			level_size = it->dnm->size_word_level;
			break;
		case DNM_LEVEL_NONE:     //avoids compiler warnings
			level_size = 0;
			break;
	}
	(it->pos)++;
	if ((it->pos < level_size) && (it->pos < it->end)) {   //next pos in bounds
		return 1;
	} else {        //iterator points to last element already
		(it->pos)--;
		return 0;
	}
}

int dnmIteratorPrevious(dnmIteratorPtr it) {
	if (it->pos > it->start) {
		(it->pos)--;
		return 1;
	} else {       //is at first element already
		return 0;
	}
}

struct dnm_chunk *getDnmChunkFromIterator(dnmIteratorPtr it) {
	if (it->pos < it->start || it->pos >= it->end) return NULL;  //out of bounds

	switch (it->level) {
		case DNM_LEVEL_PARA:
			return (it->dnm->para_level)+(it->pos);
		case DNM_LEVEL_SENTENCE:
			return (it->dnm->sent_level)+(it->pos);
		case DNM_LEVEL_WORD:
			return (it->dnm->word_level)+(it->pos);
		case DNM_LEVEL_NONE:
			return NULL;
	}

	return NULL;  //just in case
}


char *getDnmIteratorContent(dnmIteratorPtr it) {
	struct dnm_chunk *chunk = getDnmChunkFromIterator(it);

	if (chunk) {
		char *cpy = (char *)malloc(sizeof(char)*(chunk->offset_end - chunk->offset_start + 1));   //+1 for \0
		CHECK_ALLOC(cpy);
		memcpy(cpy, (it->dnm->plaintext)+(chunk->offset_start), (chunk->offset_end - chunk->offset_start));
		cpy[chunk->offset_end - chunk->offset_start] = '\0';
		return cpy;
	} else return NULL;
}

int dnmIteratorHasAnnotation(dnmIteratorPtr it, const char *annotation) {
	char * aPtr = getAnnotationPtr(it->dnm, annotation, 0);
	if (aPtr == NULL) return 0;
	else {
		//search annotation list for aPtr
		struct dnm_chunk *chunk = getDnmChunkFromIterator(it);
		size_t i = 0;
		while (i < chunk->number_of_annotations) {
			if (chunk->annotations[i++] == aPtr) return 1;
		}
		return 0;    //annotation not in list
	}
}

int dnmIteratorHasAnnotationInherited(dnmIteratorPtr it, const char *annotation) {
	char * aPtr = getAnnotationPtr(it->dnm, annotation, 0);
	if (aPtr == NULL) return 0;
	else {
		//search inherited_annotations list for aPtr
		struct dnm_chunk *chunk = getDnmChunkFromIterator(it);
		size_t i = 0;
		while (i < chunk->number_of_inherited_annotations) {
			if (chunk->inherited_annotations[i++] == aPtr) return 1;
		}
		return 0;    //annotation not in list
	}
}

void dnmIteratorAddAnnotation(dnmIteratorPtr it, const char *annotation, int writeIntoDOM, int inheritToChildren) {
	char *aPtr = getAnnotationPtr(it->dnm, annotation, 1);
	struct dnm_chunk *chunk = getDnmChunkFromIterator(it);

	//writing into DOM
	if (writeIntoDOM) {
		xmlAttr *attr;
		xmlChar *current_value = NULL;
		for (attr = chunk->dom_node->properties; attr != NULL; attr = attr->next) {
			if (xmlStrEqual(attr->name, BAD_CAST "class")) {  //annotations are stored in class attribute
				current_value = xmlNodeGetContent(attr->children);
				CHECK_ALLOC(current_value);
				//remove attr (we'll create a new one later)
				if (attr->prev != NULL)
					attr->prev->next = attr->next;
				else
					chunk->dom_node->properties = attr->next;
				if (attr->next)
					attr->next->prev = attr->prev;
				xmlFreeProp(attr);

				break;  //we're not interested in anything else
			}
		}
		size_t s = strlen((char*)current_value);
		char *new_value = (char *)malloc(sizeof(char)*(s + strlen(annotation) + 2));  //+2 for ' ' and '\0'
		CHECK_ALLOC(new_value);
		memcpy(new_value, current_value, sizeof(char)*s);
		new_value[s] = ' ';
		memcpy(new_value + s + 1, annotation, sizeof(char)*(strlen(annotation) + 1));
		xmlNewProp(chunk->dom_node, BAD_CAST "class", BAD_CAST new_value);
		free(new_value);
		xmlFree(current_value);
	}

	//write into annotations
	POSSIBLE_RESIZE(chunk->annotations, chunk->number_of_annotations, &chunk->annotations_allocated, chunk->annotations_allocated*2, char *);
	chunk->annotations[chunk->number_of_annotations++] = aPtr;

	//inherit to children
	if (inheritToChildren) {
		if (it->level == DNM_LEVEL_PARA || it->level == DNM_LEVEL_SENTENCE) { //otherwise there are no children
			dnmIteratorPtr it2 = getDnmChildrenIterator(it);
			dnmIteratorPtr it3;
			if (it2) {
				do {
					chunk = getDnmChunkFromIterator(it2);
					POSSIBLE_RESIZE(chunk->inherited_annotations, chunk->number_of_inherited_annotations,
					               &chunk->inherited_annotations_allocated, chunk->inherited_annotations_allocated*2, char *);
					chunk->inherited_annotations[chunk->number_of_inherited_annotations++] = aPtr;
					//we might have to inherit deeper
					it3 = getDnmChildrenIterator(it2);
					if (it3) {
						do {
							chunk = getDnmChunkFromIterator(it3);
							POSSIBLE_RESIZE(chunk->inherited_annotations, chunk->number_of_inherited_annotations,
							               &chunk->inherited_annotations_allocated, chunk->inherited_annotations_allocated*2, char *);
							chunk->inherited_annotations[chunk->number_of_inherited_annotations++] = aPtr;
							//no deeper level possible
						} while (dnmIteratorNext(it3));
					}
					free(it3);
				} while (dnmIteratorNext(it2));
			}
			free(it2);
		}
	}

}


//=======================================================
//Section: Creating DNM
//=======================================================


//for the creation of the DNM, we need to keep some things in mind:
struct tmp_parsedata {
	size_t plaintext_allocated;
	size_t plaintext_index;

	size_t para_level_allocated;
	size_t sent_level_allocated;
	size_t word_level_allocated;

	size_t para_level_index;
	size_t sent_level_index;
	size_t word_level_index;

	char **inherited_annotations;
	size_t inherited_annotations_allocated;
	size_t inherited_annotations_index;

	char * para_annotation;
	char * sent_annotation;
	char * word_annotation;
};

char * getAnnotationPtr(dnmPtr dnm, const char *annotation, int create) {
	struct hash_element_string *tmp;
	HASH_FIND_STR(dnm->annotation_handle, annotation, tmp);
	if (tmp == NULL) {		//Couldn't find annotation
		if (create) {
			tmp = (struct hash_element_string *)malloc(sizeof(struct hash_element_string));
			CHECK_ALLOC(tmp);
			tmp->string = strdup(annotation);
			CHECK_ALLOC(tmp->string);
			HASH_ADD_KEYPTR(hh, dnm->annotation_handle, tmp->string, strlen(tmp->string), tmp);
			return tmp->string;
		} else return NULL;
	} else {
		return tmp->string;
	}
}

char ** getAnnotationList(dnmPtr dnm, char *string, size_t * final_length, size_t * final_number) {
	char *i1 = string;
	char *i2 = string;
	int endofannotation = 0;

	char **annotationlist = (char **) malloc(8*sizeof(char *));
	CHECK_ALLOC(annotationlist);
	size_t list_length = 8;
	size_t list_index = 0;

	while (!endofannotation) {
		//go ahead to end of annotation
		while (*i2 != ' ' && *i2 != '\0') {
			i2++;
		}
		if (*i2 == '\0') endofannotation = 1;
		*i2 = '\0';
		POSSIBLE_RESIZE(annotationlist, list_index, &list_length, list_length*2, char *);
		annotationlist[list_index++] = getAnnotationPtr(dnm, i1, 1);
		i1 = ++i2;     //beginning of next annotation
	}
	*final_length = list_length;
	*final_number = list_index;
	//annotationlist = realloc(annotationlist, list_index * sizeof(char *));
	//if (list_index) CHECK_ALLOC(annotationlist);
	return annotationlist;
}

void copy_into_plaintext(const char *string, dnmPtr dnm, struct tmp_parsedata *dcs) {
	while (*string != '\0') {
		POSSIBLE_RESIZE(dnm->plaintext, dcs->plaintext_index, &dcs->plaintext_allocated,
			dcs->plaintext_allocated*2, char);
		dnm->plaintext[dcs->plaintext_index++] = *string++;
	}
}

enum dnm_level getLevelFromAnnotations(struct tmp_parsedata *dcs, char **annotationlist,
                                       size_t annotationlist_length) {
	char *tmp;
	while (annotationlist_length) {
		tmp = annotationlist[--annotationlist_length];
		if (tmp == dcs->para_annotation) return DNM_LEVEL_PARA;
		if (tmp == dcs->sent_annotation) return DNM_LEVEL_SENTENCE;
		if (tmp == dcs->word_annotation) return DNM_LEVEL_WORD;
	}
	return DNM_LEVEL_NONE;
}

void appendAnnotationsForInheritance(char **annotationlist, size_t annotationlist_number, struct tmp_parsedata *dcs) {
	POSSIBLE_RESIZE(dcs->inherited_annotations, dcs->inherited_annotations_index + annotationlist_number,
	                &(dcs->inherited_annotations_allocated), dcs->inherited_annotations_allocated*2 + annotationlist_number, char *);
	size_t i = 0;
	while (i < annotationlist_number) {
		dcs->inherited_annotations[dcs->inherited_annotations_index++] = annotationlist[i++];
	}
}

void parse_dom_into_dnm(xmlNode *n, dnmPtr dnm, struct tmp_parsedata *dcs, long parameters) {	
	xmlNode *node;
	xmlAttr *attr;
	xmlChar *tmpxmlstr;
	xmlChar *id;
	char **annotationlist;
	size_t annotationlist_length;
	size_t annotationlist_number;
	enum dnm_level level;
	struct dnm_chunk * current_chunk;
	size_t level_offset;
	size_t old_number_inherited_annotations;
	for (node = n; node!=NULL; node = node->next) {
		//possibly normalize math tags
		if ((parameters&DNM_NORMALIZE_MATH) && xmlStrEqual(node->name, BAD_CAST "math")) {
			copy_into_plaintext("[MathFormula]", dnm, dcs);
		}
		//possibly ignore math tags
		else if ((parameters&DNM_SKIP_MATH) && xmlStrEqual(node->name, BAD_CAST "math")) {

		}
		//possibly ignore cite tags
		else if ((parameters&DNM_SKIP_CITE) && xmlStrEqual(node->name, BAD_CAST "cite")) {

		}
		//copy text nodes
		else if (xmlStrEqual(node->name, BAD_CAST "text")) {

			tmpxmlstr = xmlNodeGetContent(node);
			CHECK_ALLOC(tmpxmlstr);
			copy_into_plaintext((char*)tmpxmlstr, dnm, dcs);
			xmlFree(tmpxmlstr);
		}
		//otherwise parse children recursively
		else {

//GET ANNOTATIONS AND OTHER ATTRIBUTES (IF AVAILABLE)

			id = NULL;
			annotationlist = NULL;

			for (attr = node->properties; attr != NULL; attr = attr->next) {

				if (xmlStrEqual(attr->name, BAD_CAST "id")) {
					if (id) fprintf(stderr, "Error: Multiple id attributes in one tag\n");
					id = xmlNodeGetContent(attr->children);
					CHECK_ALLOC(id);
				} else if (xmlStrEqual(attr->name, BAD_CAST "class")) {
					if (annotationlist) fprintf(stderr, "Error: Multiple class attributes in one tag\n");
					tmpxmlstr = xmlNodeGetContent(attr->children);
					CHECK_ALLOC(tmpxmlstr);
					annotationlist = getAnnotationList(dnm, (char *)tmpxmlstr, &annotationlist_length, &annotationlist_number);
					CHECK_ALLOC(annotationlist);
					xmlFree(tmpxmlstr);
				}
			}
			if (annotationlist == NULL) {  //no annotations => node is of no particular interest; handle it recursively
				parse_dom_into_dnm(node->children, dnm, dcs, parameters);
			} else {

//USING ANNOTATIONS, MAKE A dnm_chunk

				level = getLevelFromAnnotations(dcs, annotationlist, annotationlist_number);
				current_chunk = NULL;
				switch (level) {                    //take chunk from corresponding array
					case DNM_LEVEL_PARA:
						POSSIBLE_RESIZE(dnm->para_level, dcs->para_level_index, &dcs->para_level_allocated,
						                dcs->para_level_allocated*2, struct dnm_chunk);
						level_offset = dcs->para_level_index++;
						current_chunk = (dnm->para_level) + level_offset;
						break;
					case DNM_LEVEL_SENTENCE:
						POSSIBLE_RESIZE(dnm->sent_level, dcs->sent_level_index, &dcs->sent_level_allocated,
						                dcs->sent_level_allocated*2, struct dnm_chunk);
						level_offset = dcs->sent_level_index++;
						current_chunk = (dnm->sent_level) + level_offset;
						break;
					case DNM_LEVEL_WORD:
						POSSIBLE_RESIZE(dnm->word_level, dcs->word_level_index, &dcs->word_level_allocated,
						                dcs->word_level_allocated*2, struct dnm_chunk);
						level_offset = dcs->word_level_index++;
						current_chunk = (dnm->word_level) + level_offset;
						break;
					case DNM_LEVEL_NONE:
						current_chunk = NULL;
						break;
				}

				if (current_chunk != NULL) {

//WRITE VALUES INTO CHUNK

					current_chunk->id = (id == NULL ? NULL : strdup((char*)id));
					current_chunk->dom_node = node;
					current_chunk->offset_start = dcs->plaintext_index;
					current_chunk->annotations = annotationlist;
					current_chunk->number_of_annotations = annotationlist_number;
					current_chunk->annotations_allocated = annotationlist_length;

					//copy inherited annotations
					current_chunk->number_of_inherited_annotations = dcs->inherited_annotations_index;
					current_chunk->inherited_annotations_allocated = dcs->inherited_annotations_index;
					current_chunk->inherited_annotations = (char **) malloc(sizeof(char *) * dcs->inherited_annotations_index);
					CHECK_ALLOC(current_chunk->inherited_annotations);
					memcpy(current_chunk->inherited_annotations, dcs->inherited_annotations, sizeof(char *) * dcs->inherited_annotations_index);

					//level dependent stuff:

					switch (level) {
						case DNM_LEVEL_PARA:
							current_chunk->offset_children_start = dcs->sent_level_index;
							break;
						case DNM_LEVEL_SENTENCE:
							current_chunk->offset_children_start = dcs->word_level_index;
							break;
						case DNM_LEVEL_WORD:   //no children
						case DNM_LEVEL_NONE:   //no children
							current_chunk->offset_children_start = 0;
							break;
					}


					old_number_inherited_annotations = dcs->inherited_annotations_index;
					appendAnnotationsForInheritance(annotationlist, annotationlist_number, dcs);
					parse_dom_into_dnm(node->children, dnm, dcs, parameters);
					dcs->inherited_annotations_index = old_number_inherited_annotations;


					//due to realloc, location of current_chunk might change :D
					switch (level) {
						case DNM_LEVEL_PARA:
							current_chunk = (dnm->para_level) + level_offset;
							break;
						case DNM_LEVEL_SENTENCE:
							current_chunk = (dnm->sent_level) + level_offset;
							break;
						case DNM_LEVEL_WORD:
							current_chunk = (dnm->word_level) + level_offset;
							break;
						default: break;		//should never occur, but I don't like compiler warnings
					}

					//fill in remaining values
					current_chunk->offset_end = dcs->plaintext_index;

					switch (level) {
						case DNM_LEVEL_PARA:
							current_chunk->offset_children_end = dcs->sent_level_index;
							break;
						case DNM_LEVEL_SENTENCE:
							current_chunk->offset_children_end = dcs->word_level_index;
							break;
						case DNM_LEVEL_WORD:   //no child
						case DNM_LEVEL_NONE:   //no child
							current_chunk->offset_children_end = 0;
							break;
					}

				} else {   //tag isn't paragraph, sentence, or word
					old_number_inherited_annotations = dcs->inherited_annotations_index;
					appendAnnotationsForInheritance(annotationlist, annotationlist_number, dcs);
					free(annotationlist);
					parse_dom_into_dnm(node->children, dnm, dcs, parameters);
					dcs->inherited_annotations_index = old_number_inherited_annotations;
				}
			}



			xmlFree(id);
		}
	}
}

dnmPtr createDNM(xmlDocPtr doc, long parameters) {
	//check arguments
	if (doc==NULL) {
		fprintf(stderr, "dnmlib - Didn't get an xmlDoc - parse error??\n");
		return NULL;
	}

	if ((parameters&DNM_NORMALIZE_MATH) && (parameters&DNM_SKIP_MATH)) {
		fprintf(stderr, "dnmlib - got conflicting parameters (skip and normalize math)\n");
	}

	//----------------INITIALIZE----------------
	//dcs (data required only during parsing)
	struct tmp_parsedata *dcs = (struct tmp_parsedata*)malloc(sizeof(struct tmp_parsedata));
	CHECK_ALLOC(dcs);

	//dnm
	dnmPtr dnm = (dnmPtr)malloc(sizeof(struct dnm_struct));
	CHECK_ALLOC(dnm);

	dnm->document = doc;

	//plaintext
	dnm->plaintext = (char*)malloc(sizeof(char)*4096);
	CHECK_ALLOC(dnm->plaintext);

	dcs->plaintext_allocated = 4096;
	dcs->plaintext_index = 0;

	//annotations
	dnm->annotation_handle = NULL;

	dcs->para_annotation = getAnnotationPtr(dnm, "ltx_para", 1);
	dcs->sent_annotation = getAnnotationPtr(dnm, "ltx_sentence", 1);
	dcs->word_annotation = getAnnotationPtr(dnm, "ltx_word", 1);

	dcs->inherited_annotations = (char **) malloc(sizeof(char *)*64);
	dcs->inherited_annotations_allocated = 64;
	dcs->inherited_annotations_index = 0;

	//levels
	dnm->para_level = (struct dnm_chunk *) malloc(sizeof(struct dnm_chunk)*32);
	dcs->para_level_allocated = 32;
	dcs->para_level_index = 0;

	dnm->sent_level = (struct dnm_chunk *) malloc(sizeof(struct dnm_chunk)*128);
	dcs->sent_level_allocated = 128;
	dcs->sent_level_index = 0;

	dnm->word_level = (struct dnm_chunk *) malloc(sizeof(struct dnm_chunk)*1024);
	dcs->word_level_allocated = 1024;
	dcs->word_level_index = 0;



	//Call the actual parsing function
	parse_dom_into_dnm(xmlDocGetRootElement(doc), dnm, dcs, parameters);


	//----------------CLEAN UP----------------

	//end plaintext with \0 and truncate array
	POSSIBLE_RESIZE(dnm->plaintext, dcs->plaintext_index, &dcs->plaintext_allocated,
			dcs->plaintext_allocated+1, char);
	dnm->plaintext[dcs->plaintext_index++] = '\0';
	dnm->size_plaintext = dcs->plaintext_index;
	dnm->plaintext = realloc(dnm->plaintext, dcs->plaintext_index * sizeof(char));
	if (dnm->size_plaintext) CHECK_ALLOC(dnm->plaintext);

	//truncate level lists
	dnm->size_para_level = dcs->para_level_index;
	dnm->para_level = realloc(dnm->para_level, dnm->size_para_level * sizeof(struct dnm_chunk));
	if (dnm->size_para_level) CHECK_ALLOC(dnm->para_level);    //only check if memory has actually been allocated
	dnm->size_sent_level = dcs->sent_level_index;
	dnm->sent_level = realloc(dnm->sent_level, dnm->size_sent_level * sizeof(struct dnm_chunk));
	if (dnm->size_sent_level) CHECK_ALLOC(dnm->sent_level);
	dnm->size_word_level = dcs->word_level_index;
	dnm->word_level = realloc(dnm->word_level, dnm->size_word_level * sizeof(struct dnm_chunk));
	if (dnm->size_word_level) CHECK_ALLOC(dnm->word_level);

	free(dcs->inherited_annotations);
	free(dcs);

	return dnm;
}



//=======================================================
//Section: Freeing DNM
//=======================================================

void freeLevelList(struct dnm_chunk * array, size_t size) {
	while (size--) {
		free(array[size].id);
		free(array[size].annotations);
		free(array[size].inherited_annotations);
	}
	free(array);
}

void freeDNM(dnmPtr dnm) {
	//free annotations
	struct hash_element_string *current, *tmp;
	HASH_ITER(hh, dnm->annotation_handle, current, tmp) {
		HASH_DEL(dnm->annotation_handle, current);
		free(current->string);
		free(current);
	}
	//free plaintext
	free(dnm->plaintext);
	//free level lists
	freeLevelList(dnm->para_level, dnm->size_para_level);
	freeLevelList(dnm->sent_level, dnm->size_sent_level);
	freeLevelList(dnm->word_level, dnm->size_word_level);

	free(dnm);
}
