#ifndef LLAMAPUN_TOKENIZER_H
#define LLAMAPUN_TOKENIZER_H

// Some everyday C includes
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include <pcre.h>
#include "stopwords.h"
#include "dnmlib.h"
// Senna POS tagger
#include <senna/SENNA_utils.h>
#include <senna/SENNA_Hash.h>
#include <senna/SENNA_Tokenizer.h>
#include <senna/SENNA_POS.h>

dnmRanges tokenize_sentences(char* text);
dnmRanges tokenize_words(dnmRange sentence, char* text);
void display_ranges(dnmRanges ranges, char* text);
dnmRange trim_range(dnmRange range, char* text);

// SENNA loading (how do we make this thread safe?)
void initialize_tokenizer(const char *opt_path);
void free_tokenizer();
#endif
