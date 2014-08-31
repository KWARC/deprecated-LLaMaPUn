#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include <pcre.h>
#include "tokenizer.h"
#include "stopwords.h"
#include "dnmlib.h"
// Senna POS tagger
#include <SENNA_utils.h>
#include <SENNA_Hash.h>
#include <SENNA_Tokenizer.h>
#include <SENNA_POS.h>

#define abbreviation_pattern "\\W(?:C(?:[ft]|o(?:n[jn]|lo?|rp)?|a(?:l(?:if)?|pt)|mdr|p?l|res)|M(?:[dst]|a(?:[jnry]|ss)|i(?:ch|nn|ss)|o(?:nt)?|ex?|rs?)|A(?:r(?:[ck]|iz)|l(?:t?a)?|ttys?|ssn|dm|pr|ug|ve)|c(?:o(?:rp|l)?|(?:ap)?t|mdr|p?l|res|f)|S(?:e(?:ns?|pt?|c)|(?:up|g)?t|ask|r)|s(?:e(?:ns?|pt?|c)|(?:up|g)?t|r)|a(?:ttys?|ssn|dm|pr|rc|ug|ve|l)|P(?:enna?|-a.s|de?|lz?|rof|a)|D(?:e(?:[cfl]|p?t)|ist|ak|r)|I(?:[as]|n[cd]|da?|.e|ll)|F(?:e[bd]|w?y|ig|la|t)|O(?:k(?:la)?|[cn]t|re)|d(?:e(?:p?t|c)|ist|r)|E(?:xpy?|.g|sp|tc|q)|R(?:e(?:ps?|sp|v)|d)|T(?:e(?:nn|x)|ce|hm)|e(?:xpy?|.g|sp|tc|q)|m(?:[st]|a[jry]|rs?)|r(?:e(?:ps?|sp|v)|d)|N(?:e(?:br?|v)|ov?)|W(?:isc?|ash|yo?)|f(?:w?y|eb|ig|t)|p(?:de?|lz?|rof)|J(?:u[ln]|an|r)|U(?:SAFA|niv|t)|j(?:u[ln]|an|r)|K(?:ans?|en|y)|B(?:lv?d|ros)|b(?:lv?d|ros)|G(?:en|ov|a)|L(?:td?|a)|g(?:en|ov)|i(?:.e|nc)|l(?:td?|a)|[Hh]wa?y|V[ast]|Que|nov?|univ|Yuk|oct|tce|vs)$"
#define math_formula_pattern "^\\s*MathFormula\\s*\\n"
#define max_abbreviation_length 6
#define sentence_termination_delimiters ".?!\n"
#define OVECCOUNT 30    /* should be a multiple of 3 */

SENNA_Hash *word_hash, *caps_hash, *suff_hash, *gazt_hash, *gazl_hash, *gazm_hash, *gazo_hash, *gazp_hash;
SENNA_Tokenizer *tokenizer_active, *tokenizer_passive;
void initialize_tokenizer(const char *opt_path) {
  /* Initialize SENNA toolkit components: */
  /* SENNA inputs */
  word_hash = SENNA_Hash_new(opt_path, "hash/words.lst");
  caps_hash = SENNA_Hash_new(opt_path, "hash/caps.lst");
  suff_hash = SENNA_Hash_new(opt_path, "hash/suffix.lst");
  gazt_hash = SENNA_Hash_new(opt_path, "hash/gazetteer.lst");

  gazl_hash = SENNA_Hash_new_with_admissible_keys(opt_path, "hash/ner.loc.lst", "data/ner.loc.dat");
  gazm_hash = SENNA_Hash_new_with_admissible_keys(opt_path, "hash/ner.msc.lst", "data/ner.msc.dat");
  gazo_hash = SENNA_Hash_new_with_admissible_keys(opt_path, "hash/ner.org.lst", "data/ner.org.dat");
  gazp_hash = SENNA_Hash_new_with_admissible_keys(opt_path, "hash/ner.per.lst", "data/ner.per.dat");
  /* Tokenizer */
  // We turn the SENNA smart tokenization on with "0", but we should take care, as the word ranges vary badly.
  // TODO: Read up on how SENNA requires its word tokens split for further processing, see if there is a sane setup
  tokenizer_active = SENNA_Tokenizer_new(word_hash, caps_hash, suff_hash, gazt_hash, gazl_hash, gazm_hash, gazo_hash, gazp_hash, 0);
  tokenizer_passive = SENNA_Tokenizer_new(word_hash, caps_hash, suff_hash, gazt_hash, gazl_hash, gazm_hash, gazo_hash, gazp_hash, 1);
}
void free_tokenizer() {
  // Release the stopwords
  free_stopwords();
  // Release SENNA structures
  SENNA_Tokenizer_free(tokenizer_active);
  SENNA_Tokenizer_free(tokenizer_passive);

  SENNA_Hash_free(word_hash);
  SENNA_Hash_free(caps_hash);
  SENNA_Hash_free(suff_hash);
  SENNA_Hash_free(gazt_hash);

  SENNA_Hash_free(gazl_hash);
  SENNA_Hash_free(gazm_hash);
  SENNA_Hash_free(gazo_hash);
  SENNA_Hash_free(gazp_hash);
}

void display_ranges(char* text, dnmRanges ranges) {
  int i;
  for (i=0; i<ranges.length; i++) {
    int start = ranges.range[i].start;
    int end = ranges.range[i].end;
    fprintf(stderr,"%d. %.*s\n",i,end-start+1,text+start);
  }
}

dnmRange trim_range(char* text, dnmRange range) {
  while(isspace(text[range.start])) {
    range.start++;
  }
  while(isspace(text[range.end])) {
    range.end--;
  }
  return range;
}

bool has_alnum(char* text, dnmRange range) {
  unsigned int index;
  bool alnum_found = false;
  for (index=range.start; index<=range.end; index++) {
    if (isalnum(*(text+index))) {
      alnum_found = true;
      break;
    }
  }
  return alnum_found;
}
bool is_range_stopword(char* text, dnmRange range) {
  char* word = plain_range_to_string(text, range);
  int stopword_check = is_stopword(word);
  free(word);
  return (bool) stopword_check;
}

dnmRanges tokenize_sentences(char* text) {
  load_stopwords(); // We need to filter against the stopwords
  const char* error;
  int error_offset;
  pcre *abbreviation_regex, *math_formula_regex;
  abbreviation_regex = pcre_compile(abbreviation_pattern, 0, &error, &error_offset, NULL);
  math_formula_regex = pcre_compile(math_formula_pattern, 0, &error, &error_offset, NULL);
  dnmRanges sentence_ranges;
  unsigned int allocated_ranges = 512;
  sentence_ranges.range = malloc(allocated_ranges * sizeof(dnmRange));
  sentence_ranges.length = 0;
  char* copy = text+2; // Sentences have at least two characters (and we avoid invalid reads )
  unsigned int sentence_count = 0;
  unsigned int start_sentence = 0;
  unsigned int end_sentence = 2;
  bool has_content = false;
  while ((copy != NULL) && ((*copy) != '\0')) {
    if (sentence_count > allocated_ranges-2) {
      // Incrementally grow our sentence array in memory, when needed
      allocated_ranges *= 2;
      sentence_ranges.range = (dnmRange*) realloc(sentence_ranges.range, allocated_ranges * sizeof(dnmRange));
    }
    switch (*copy) {
      case '.':
        // Baseline condition - only split when we have a following uppercase letter
        ; char* next_letter = copy+1;
        // Get next non-space character
        while (isspace(*next_letter)) { next_letter++; }
        // Uppercase next?
        if (!isupper(*next_letter)) {
          copy++;
          end_sentence++;
          break;
        }
        // Ok, uppercase, but is it a stopword? If so, we must ALWAYS break the sentence:
        int next_word_length = 0;
        char next_word[20];
        while(*next_letter && (isalpha(*next_letter)) && (next_word_length<20)) {
          next_word[next_word_length++] = *next_letter;
          next_letter++;
        }
        next_word[next_word_length] = '\0';
        if (!is_stopword(next_word)) { // Else case continues in the !? cases and will sentence break
          // Check for abbreviations:
          // Longest abbreviation is 6 characters, so take window of 7 chars to the left
          char prior_context[8] = {*(copy-7), *(copy-6), *(copy-5), *(copy-4), *(copy-3), *(copy-2), *(copy-1),'\0'};
          int abbrev_vector[OVECCOUNT];
          int abbrev_rc = pcre_exec(abbreviation_regex, NULL, prior_context,      7, 0, 0, abbrev_vector, OVECCOUNT);
          if (abbrev_rc >= 0) { // Abbreviation, just skip over char:
            copy++;
            end_sentence++;
            break;
          }
          // Other skippable cases:
          if ( isalpha(*(copy-1)) && (!isalnum(*(copy-2))) && (*(copy-1) != 'I') ) {
            // Don't consider single letters followed by a punctuation sign an end of a sentence, if the next char is not a space/line break.
            // also "a.m." and "p.m." shouldn't get split
            copy++;
            end_sentence++;
            break;
          }
          //TODO: Handle dot-dot-dot "..."
        }
      case '?':
      case '!':
        // Or quote-enclosed punctuation: "." "?" "!"
        if  ( ((*(copy-1) == '"') && (*(copy+1) == '"')) ||
              ((*(copy-1) == '[') && (*(copy+1) == ']')) ||
              ((*(copy-1) == '(') && (*(copy+1) == ')')) ||
              ((*(copy-1) == '{') && (*(copy+1) == '}')) ) {
          copy++;
          end_sentence++;
          break;
        }
        // None of our special cases was matched, we have a
        // REAL SENTENCE BREAK:
        // Save sentence, if content is present:
        if (has_content && (end_sentence-start_sentence>2)) {
          sentence_ranges.range[sentence_count].start = start_sentence;
          sentence_ranges.range[sentence_count].end = end_sentence;
          sentence_count++;
        }
        // Init next sentence start:
        copy++;
        start_sentence = ++end_sentence;
        has_content=false;
        // End of sentence punctuation case
        break;

      case '\n': // Multiple new lines are a paragraph break, UNLESS MathFormula is in between
        if (*(copy-1) == '\n') {
          // Look ahead for MathFormula, end sentence if not found
          int ovector[OVECCOUNT];
          int rc = pcre_exec(math_formula_regex, NULL, copy, strlen(copy), 0, 0, ovector, OVECCOUNT);
          if (rc < 0) {
            // No match, sentence break here
            // All follow-up whitespace should be taken as part of this sentence:
            while (isspace(*(copy+1))) {
              copy++;
              end_sentence++;
            }
            // Save sentence, if content is present:
            if (has_content && (end_sentence-start_sentence>2)) {
              sentence_ranges.range[sentence_count].start = start_sentence;
              sentence_ranges.range[sentence_count].end = end_sentence;
              sentence_count++;
            }
            // Init next sentence start:
            copy++;
            start_sentence = ++end_sentence;
            has_content=false;
          }
          else {
            // MathFormula match, so don't break the sentence and move cursor to after math (if we have content)
            copy += ovector[1];
            end_sentence += ovector[1];
            if ((! has_content) || (isupper(*copy))) {
              // But if we have no content (stand-alone formula),
              // OR if the next letter is capital (math sentence terminator),
              //  make a sentence break and save:
              if (has_content && (end_sentence-start_sentence>1)) {
                sentence_ranges.range[sentence_count].start = start_sentence;
                sentence_ranges.range[sentence_count].end = end_sentence-1;
                sentence_count++;
              }
              // Init next sentence start:
              start_sentence = end_sentence;
              has_content=false;
            }
          }
        }
        else {
          // Neutral otherwise:
          copy++;
          end_sentence++;
        }
        // End of line break Case
        break;
      default: //just continue if a regular character
        // TODO: Special case for quote-based sentence breaks, namely ': "'
        //     An example is cited here: " This is an example of the phenomenon."

        // spaces are essentially neutral characters
        if ((!has_content) && (isalnum(*copy))) {
          has_content=true;
        }
        copy++;
        end_sentence++;
    } }
  sentence_ranges.length = sentence_count;
  // Trim all sentences:
  int index;
  for (index=0; index<sentence_ranges.length; index++) {
    sentence_ranges.range[index] = trim_range(text, sentence_ranges.range[index]);
  }
  //Developer demo:
  //   display_ranges(sentence_ranges, text);
  pcre_free(abbreviation_regex);
  pcre_free(math_formula_regex);
  return sentence_ranges;
}

dnmRanges tokenize_words(char* text, dnmRange sentence_range, long parameters) {
  char* sentence = plain_range_to_string(text, sentence_range);

  // Tokenize with SENNA:
  if (tokenizer_active == NULL) {
    fprintf(stderr,"Please first initialize tokenizer before running tokenize_words!\n");
    exit(1);
  }
  SENNA_Tokens* tokens = SENNA_Tokenizer_tokenize(tokenizer_active, sentence);

  dnmRanges word_ranges;
  word_ranges.length = 0;
  word_ranges.range = malloc(tokens->n * sizeof(dnmRange));
  int token_index;
  for(token_index = 0; token_index < tokens->n; token_index++) {
    dnmRange current_word;
    current_word.start = sentence_range.start + tokens->start_offset[token_index];
    current_word.end = sentence_range.start + tokens->end_offset[token_index] - 1;
    current_word = trim_range(text, current_word);

    bool word_accepted = true; // Accept all by default
    if ((!parameters) || (parameters&TOKENIZER_ACCEPT_ALL)) {
      word_accepted = true; }
    else {
      if (word_accepted && (parameters&TOKENIZER_ALPHANUM_ONLY) && !(has_alnum(text, current_word))) {
        // Not alphanumeric, negative filter triggered:
          word_accepted = false; }
      if (word_accepted && (parameters&TOKENIZER_FILTER_STOPWORDS) && is_range_stopword(text,current_word)) {
          // Filtering stopwords AND stopword found -- negative filter triggered:
          word_accepted = false; }
     }
    if (word_accepted) {
      word_ranges.range[word_ranges.length++] = current_word;
    }
  }
  free(sentence);
  return word_ranges;
}
