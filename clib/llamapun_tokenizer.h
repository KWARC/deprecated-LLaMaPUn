// Some everyday C includes
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include <pcre.h>
#include "dnmlib.h"

dnmRanges tokenize_sentences(char* text);
void display_sentences(dnmRanges sentence_offsets, char* text);
dnmRange trim_sentence(dnmRange range, char* text);
