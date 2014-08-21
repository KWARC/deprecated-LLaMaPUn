// Some everyday C includes
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "dnmlib.h"
#include <pcre.h>

dnmRanges tokenize_sentences(char* text);
void display_sentences(char* text, dnmRanges sentence_offsets);
