// Some everyday C includes
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "old_dnmlib.h"
#include <pcre.h>

old_dnm_offset* tokenize_sentences(char* text);
void display_sentences(char* text, old_dnm_offsets* sentence_offsets);
