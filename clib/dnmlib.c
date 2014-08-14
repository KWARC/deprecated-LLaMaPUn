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

#define NUMBER_OF_EXTENDED_PARA_ANNOTATIONS 5


//=======================================================
//Section: Iterators
//=======================================================

char * getAnnotationPtr(dnmPtr dnm, const char *annotation, int create);

dnmIteratorPtr getDnmIterator(dnmPtr dnm, enum dnm_level level) {
	/* returns an iterator over dnm on the specified level
	   memory allocated for the iterator has to be free'd */
	struct dnm_iterator * it = (struct dnm_iterator *)malloc(sizeof(struct dnm_iterator));
	CHECK_ALLOC(it);

	//set iterator properties
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
		case DNM_LEVEL_NONE:
			it->end = 0;
			break;
	}
	return it;
}

dnmIteratorPtr getDnmChildrenIterator(dnmIteratorPtr it) {
	/* returns iterator for the children of the current position of it,
	   i.e. over the sentences of a certain paragraph, or over the words of a sentence
	   memory allocated for new iterator has to be free'd manually as well */
	struct dnm_iterator * new_it = (struct dnm_iterator *)malloc(sizeof(struct dnm_iterator));
	CHECK_ALLOC(new_it);

	//find the dnm chunk corresponding to it
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

	//set the properties of the children iterator
	new_it->dnm = it->dnm;
	new_it->pos = chunk->offset_children_start;
	new_it->start = chunk->offset_children_start;
	new_it->end = chunk->offset_children_end;

	//return NULL if there are no children
	if (new_it->start >= new_it->end) {
		free(new_it);
		return NULL;
	}

	return new_it;
}

int dnmIteratorNext(dnmIteratorPtr it) {
	/* makes the iterator point to the next element of the document
	   returns 0, if there is no element left, otherwise 1 */

	//determine number of elements, which obviously depends on the level
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
		case DNM_LEVEL_NONE:
			level_size = 0;
			break;
	}
	//check, whether the next position would be in the bounds
	(it->pos)++;
	if ((it->pos < level_size) && (it->pos < it->end)) {
		return 1;
	} else {        //iterator points to last element already
		(it->pos)--;
		return 0;
	}
}

int dnmIteratorPrevious(dnmIteratorPtr it) {
	/* like dnmIteratorNext, just the other direction */

	//decrease, iff new position would be in bounds
	if (it->pos > it->start) {
		(it->pos)--;
		return 1;
	} else {
		return 0;
	}
}

struct dnm_chunk *getDnmChunkFromIterator(dnmIteratorPtr it) {
	/*returns a pointer to the dnm chunk corresponding to the current iterator position
	  not intended for external calls */

	if (it->pos < it->start || it->pos >= it->end) return NULL;  //out of bounds

	//of course, the chunk location is level dependent
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

	return NULL;  //just in case, some levels get added
}


char *getDnmIteratorContent(dnmIteratorPtr it) {
	/* returns the plaintext corresponding to the current iterator position.
	   The string, which is ended by \0, has to be freed manually. */

	struct dnm_chunk *chunk = getDnmChunkFromIterator(it);

	if (chunk) {
		//create and return a copy of the plaintext fragment
		char *cpy = (char *)malloc(sizeof(char)*(chunk->offset_end - chunk->offset_start + 1));   //+1 for \0
		CHECK_ALLOC(cpy);
		memcpy(cpy, (it->dnm->plaintext)+(chunk->offset_start), (chunk->offset_end - chunk->offset_start));
		cpy[chunk->offset_end - chunk->offset_start] = '\0';
		return cpy;
	} else return NULL;
}

int dnmIteratorHasAnnotation(dnmIteratorPtr it, const char *annotation) {
	/* Checks, whether the element, pointed to by the iterator, has the annotation.
	   Returns 1, if it has the annotation, otherwise 0.
	   Note that there is a faster way, if you want to repeatedly check for one specific annotation.
	   (by determining aPtr just once, feature isn't implemented yet) */

	char * aPtr = getAnnotationPtr(it->dnm, annotation, 0);
	if (aPtr == NULL) return 0;    //annotation doesn't exist at all
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
	/* like dnmIteratorHasAnnotation, just for inherited annotations.
	   Inherited annotations are annotations of the parent nodes */
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
	/* writes adds an annotation to the dnm chunk the iterator refers to.
	   The annotation can also be written into the DOM, and inherited to the child chunks.
	   Again, note that there is a faster way for doing this repeatedly with an annotation,
	   by determining aPtr just once. This feature isn't implemented yet */

	char *aPtr = getAnnotationPtr(it->dnm, annotation, 1);
	struct dnm_chunk *chunk = getDnmChunkFromIterator(it);

	//writing into DOM
	if (writeIntoDOM) {
		//search for class attribute
		xmlAttr *attr;
		xmlChar *current_value = NULL;
		for (attr = chunk->dom_node->properties; attr != NULL; attr = attr->next) {
			if (xmlStrEqual(attr->name, BAD_CAST "class")) {  //annotations are stored in class attribute
				current_value = xmlNodeGetContent(attr->children);
				CHECK_ALLOC(current_value);
				//remove attr (we'll create and add a new one later)
				if (attr->prev != NULL)
					attr->prev->next = attr->next;
				else
					chunk->dom_node->properties = attr->next;
				if (attr->next)
					attr->next->prev = attr->prev;
				xmlFreeProp(attr);

				break;  //we're not interested in other attributes
			}
		}

		//create new value string (by appending the new annotation to the old value)
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

	//write into annotations of chunk
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
					//we might have to inherit one level deeper
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
//Section: Tokenization
//=======================================================

void freeLevelList(struct dnm_chunk * array, size_t size);  //defined below

void markSentences(dnmPtr dnm, size_t start_offsets[], size_t end_offsets[], size_t n) {
	if (dnm->size_sent_level) {
		fprintf(stderr, "There are already sentences marked up in the DNM - forgetting them\n");
		freeLevelList(dnm->sent_level, dnm->size_sent_level);
	}
	dnm->size_sent_level = n;
	dnm->sent_level = (struct dnm_chunk *) malloc(n*sizeof(struct dnm_chunk));

	size_t index;
	char tmp_idstring[512];
	for (index = 0; index < n; index++) {
		snprintf(tmp_idstring, sizeof(tmp_idstring), "sentence.%ld", index);
		dnm->sent_level[index].id = strdup(tmp_idstring);
		dnm->sent_level[index].dom_node = NULL;    //Need to fix this if we're creating tags in xhtml
		dnm->sent_level[index].level = DNM_LEVEL_SENTENCE;
		dnm->sent_level[index].offset_parent = -1;
		dnm->sent_level[index].offset_children_start = -1;
		dnm->sent_level[index].offset_children_end = -1;

		dnm->sent_level[index].annotations = NULL;
		dnm->sent_level[index].inherited_annotations = NULL;   //Should fix that later...
		dnm->sent_level[index].number_of_annotations = 0;
		dnm->sent_level[index].number_of_inherited_annotations = 0;
		dnm->sent_level[index].annotations_allocated = 0;
		dnm->sent_level[index].inherited_annotations_allocated = 0;

		dnm->sent_level[index].offset_start = start_offsets[index];
		dnm->sent_level[index].offset_end = end_offsets[index];
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

	enum dnm_level current_level;

	char * para_annotation;
	char * sent_annotation;
	char * word_annotation;
	char * extended_para_annotations[NUMBER_OF_EXTENDED_PARA_ANNOTATIONS];
};


char * getAnnotationPtr(dnmPtr dnm, const char *annotation, int create) {
	/* returns the (unique) pointer to the annotation, if create is non-zero,
	   it creates the annotation, if it doesn't exist yet.
	   These pointers serve as ids */

	//search for annotation in hash
	struct hash_element_string *tmp;
	HASH_FIND_STR(dnm->annotation_handle, annotation, tmp);

	//if it doesn't exist yet, create it (if the parameter is set)
	if (tmp == NULL) {
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
	/* Returns an array of annotation pointers, extracted from string.
	   (In XHTML, the class attribute has the annotations separated by whitespaces)
	   The pointers final_length and final_number, represent the amount of memory
	   allocated, and the number of annotations detected. */

	char *i1 = string;
	char *i2 = string;
	int reached_end = 0;

	char **annotationlist = (char **) malloc(8*sizeof(char *));
	CHECK_ALLOC(annotationlist);
	size_t list_length = 8;
	size_t list_index = 0;

	//loop over annotations
	while (!reached_end) {
		//go ahead to end of current annotation
		while (*i2 != ' ' && *i2 != '\0') {
			i2++;
		}
		if (*i2 == '\0') reached_end = 1;
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
	/* appends string to the plaintext, stored in dnm */
	while (*string != '\0') {
		POSSIBLE_RESIZE(dnm->plaintext, dcs->plaintext_index, &dcs->plaintext_allocated,
			dcs->plaintext_allocated*2, char);
		dnm->plaintext[dcs->plaintext_index++] = *string++;
	}
}

enum dnm_level getLevelFromAnnotations(struct tmp_parsedata *dcs, char **annotationlist,
                                       size_t annotationlist_length) {
	/* returns the level, according to the list of annotation pointers. */

	char *tmp;
	while (annotationlist_length) {
		tmp = annotationlist[--annotationlist_length];
		if (tmp == dcs->para_annotation) return DNM_LEVEL_PARA;
		if (tmp == dcs->sent_annotation) return DNM_LEVEL_SENTENCE;
		if (tmp == dcs->word_annotation) return DNM_LEVEL_WORD;
	}
	//if no annotation declaring the level was found:
	return DNM_LEVEL_NONE;
}

enum dnm_level getLevelFromAnnotations_extendedPara(struct tmp_parsedata *dcs, char **annotationlist,
                                       size_t annotationlist_length) {
	/* returns level para, if the annotation pointers fall into the extended range (toc, bibliography, titles, ...) */

	char *tmp;
	int i;
	while (annotationlist_length) {
		tmp = annotationlist[--annotationlist_length];
		//iterate over annotations marking paragraphs in extended range
		i = NUMBER_OF_EXTENDED_PARA_ANNOTATIONS;
		while (i--) {
			if (dcs->extended_para_annotations[i] == tmp) return DNM_LEVEL_PARA;
		}
	}

	//if nothing was found:
	return DNM_LEVEL_NONE;
}

void appendAnnotationsForInheritance(char **annotationlist, size_t annotationlist_number, struct tmp_parsedata *dcs) {
	/* appends annotations to the list in the dcs, which accumulates the annotations of the parent tags. */
	POSSIBLE_RESIZE(dcs->inherited_annotations, dcs->inherited_annotations_index + annotationlist_number,
	                &(dcs->inherited_annotations_allocated), dcs->inherited_annotations_allocated*2 + annotationlist_number, char *);
	size_t index = 0;
	while (index < annotationlist_number) {
		dcs->inherited_annotations[dcs->inherited_annotations_index++] = annotationlist[index++];
	}
}

void parse_dom_into_dnm(xmlNode *n, dnmPtr dnm, struct tmp_parsedata *dcs, long parameters) {	
	/* the core function which (recursively) parses the DOM into the DNM */

	//declaring a lot of variables required later on...
	xmlNode *node;
	xmlAttr *attr;
	xmlChar *tmpxmlstr;
	xmlChar *id;
	char **annotationlist;
	size_t annotationlist_length;
	size_t annotationlist_number;
	enum dnm_level level;
	enum dnm_level tmplevel;
	struct dnm_chunk * current_chunk;
	size_t level_offset;
	size_t old_number_inherited_annotations;

	//iterate over nodes
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
		//copy contents of text nodes into plaintext
		else if (xmlStrEqual(node->name, BAD_CAST "text")) {

			tmpxmlstr = xmlNodeGetContent(node);
			CHECK_ALLOC(tmpxmlstr);
			copy_into_plaintext((char*)tmpxmlstr, dnm, dcs);
			xmlFree(tmpxmlstr);
		}
		//otherwise, parse children recursively
		else {

//GET ANNOTATIONS AND OTHER ATTRIBUTES (IF AVAILABLE)

			id = NULL;
			annotationlist = NULL;

			//iterate over attributes (we're looking for the id and the class values)
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

//USING THE ANNOTATIONS, MAKE A dnm_chunk

				level = getLevelFromAnnotations(dcs, annotationlist, annotationlist_number);
				if (level==DNM_LEVEL_NONE && (dcs->current_level == DNM_LEVEL_NONE) && (parameters & DNM_EXTEND_PARA_RANGE)) {
					level = getLevelFromAnnotations_extendedPara(dcs, annotationlist, annotationlist_number);
				}

				// Check that level complies with strict hierarchy
				if ( (dcs->current_level == DNM_LEVEL_PARA && level == DNM_LEVEL_PARA) ||
					  (dcs->current_level == DNM_LEVEL_SENTENCE && (level!=DNM_LEVEL_WORD && level!=DNM_LEVEL_NONE)) ||
					  (dcs->current_level == DNM_LEVEL_WORD && level != DNM_LEVEL_NONE)) {
					fprintf(stderr, "There are violations in the document hierarchy!\n");
				}

				tmplevel = dcs->current_level;
				if (level != DNM_LEVEL_NONE) dcs->current_level = level;

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

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
					//recursively parse children
					parse_dom_into_dnm(node->children, dnm, dcs, parameters);
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

					//"remove" annotations from this chunk in the inheritance list
					dcs->inherited_annotations_index = old_number_inherited_annotations;

					//switch level back
					dcs->current_level = tmplevel;

					//due to realloc, location of current_chunk might have changed :D
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

				} else {   //tag isn't paragraph, sentence, or word => we don't create a chunk
					old_number_inherited_annotations = dcs->inherited_annotations_index;
					appendAnnotationsForInheritance(annotationlist, annotationlist_number, dcs);
					free(annotationlist);
					//recursively parse children
					parse_dom_into_dnm(node->children, dnm, dcs, parameters);
					//"remove" annotations from this node again from the list
					dcs->inherited_annotations_index = old_number_inherited_annotations;
				}
			}

			xmlFree(id);
		}
	}
}

dnmPtr createDNM(xmlDocPtr doc, long parameters) {
	/* Creates a DNM and returns a pointer to it.
	   The memory has to be free'd later by calling freeDNM */

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
	dnm->parameters = parameters;

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

	if (parameters & DNM_EXTEND_PARA_RANGE) {
		dcs->extended_para_annotations[0] = getAnnotationPtr(dnm, "ltx_title", 1);
		dcs->extended_para_annotations[1] = getAnnotationPtr(dnm, "ltx_authors", 1);
		//there are ltx_p annotations inside tags annotated as ltx_para, but there is a mechanism to ignore those
		dcs->extended_para_annotations[2] = getAnnotationPtr(dnm, "ltx_p", 1);
		//again occurs inside paragraphs etc., but required e.g. for table of contents ("ltx_tocentry"s are nested)
		dcs->extended_para_annotations[3] = getAnnotationPtr(dnm, "ltx_text", 1);
		dcs->extended_para_annotations[4] = getAnnotationPtr(dnm, "ltx_bibitem", 1);
	}

	dcs->inherited_annotations = (char **) malloc(sizeof(char *)*64);
	dcs->inherited_annotations_allocated = 64;
	dcs->inherited_annotations_index = 0;

	//levels
	dcs->current_level = DNM_LEVEL_NONE;

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
 
	//after the creation process, we don't need the dcs anymore
	free(dcs->inherited_annotations);
	free(dcs);

	return dnm;
}



//=======================================================
//Section: Freeing DNM
//=======================================================

void freeLevelList(struct dnm_chunk * array, size_t size) {
	/* frees the level list */
	while (size--) {
		free(array[size].id);
		free(array[size].annotations);
		free(array[size].inherited_annotations);
	}
	free(array);
}

void freeDNM(dnmPtr dnm) {
	/* frees the dnm */
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
