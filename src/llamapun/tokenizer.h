/*! \defgroup tokenizer Tokenizer
    The LLaMaPUn tokenization implements:
     - sentence segmentation for STEM documents
     - word tokenization, via the SENNA Tokenizer
     @file
*/
#ifndef LLAMAPUN_TOKENIZER_H
#define LLAMAPUN_TOKENIZER_H

/*! Accept all word tokens that SENNA detects (default) */
#define TOKENIZER_ACCEPT_ALL (1 << 0)
/*! Only accept word tokens that have some alphanumeric content */
#define TOKENIZER_ALPHANUM_ONLY (1 << 1)
/*! Only accept non-stopwords as word tokens */
#define TOKENIZER_FILTER_STOPWORDS (1 << 2)

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
dnmRanges tokenize_words(char* text, dnmRange sentence, long parameters);

// SENNA loading (how do we make this thread safe?)
void initialize_tokenizer(const char *opt_path);
void free_tokenizer();

// Utility function:
bool has_alnum(char* text, dnmRange range);
dnmRange trim_range(char* text, dnmRange range);
void display_ranges(char* text, dnmRanges ranges);
bool is_range_stopword(char* text, dnmRange range);
#endif
