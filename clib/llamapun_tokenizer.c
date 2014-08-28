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

#define abbreviation_pattern "\\W(?:C(?:[ft]|o(?:n[jn]|lo?|rp)?|a(?:l(?:if)?|pt)|mdr|p?l|res)|M(?:[dst]|a(?:[jnry]|ss)|i(?:ch|nn|ss)|o(?:nt)?|ex?|rs?)|A(?:r(?:[ck]|iz)|l(?:t?a)?|ttys?|ssn|dm|pr|ug|ve)|c(?:o(?:rp|l)?|(?:ap)?t|mdr|p?l|res|f)|S(?:e(?:ns?|pt?|c)|(?:up|g)?t|ask|r)|s(?:e(?:ns?|pt?|c)|(?:up|g)?t|r)|a(?:ttys?|ssn|dm|pr|rc|ug|ve|l)|P(?:enna?|-a.s|de?|lz?|rof|a)|D(?:e(?:[cfl]|p?t)|ist|ak|r)|I(?:[as]|n[cd]|da?|.e|ll)|F(?:e[bd]|w?y|ig|la|t)|O(?:k(?:la)?|[cn]t|re)|d(?:e(?:p?t|c)|ist|r)|E(?:xpy?|.g|sp|tc|q)|R(?:e(?:ps?|sp|v)|d)|T(?:e(?:nn|x)|ce|hm)|e(?:xpy?|.g|sp|tc|q)|m(?:[st]|a[jry]|rs?)|r(?:e(?:ps?|sp|v)|d)|N(?:e(?:br?|v)|ov?)|W(?:isc?|ash|yo?)|f(?:w?y|eb|ig|t)|p(?:de?|lz?|rof)|J(?:u[ln]|an|r)|U(?:SAFA|niv|t)|j(?:u[ln]|an|r)|K(?:ans?|en|y)|B(?:lv?d|ros)|b(?:lv?d|ros)|G(?:en|ov|a)|L(?:td?|a)|g(?:en|ov)|i(?:.e|nc)|l(?:td?|a)|[Hh]wa?y|V[ast]|Que|nov?|univ|Yuk|oct|tce|vs)$"
#define math_formula_pattern "^\\s*MathFormula\\s*\\n"
#define max_abbreviation_length 6
#define sentence_termination_delimiters ".?!\n"
#define OVECCOUNT 30    /* should be a multiple of 3 */

SENNA_Hash *word_hash, *caps_hash, *suff_hash, *gazt_hash, *gazl_hash, *gazm_hash, *gazo_hash, *gazp_hash;
SENNA_Tokenizer *tokenizer;
void initialize_tokenizer() {
  /* Initialize SENNA toolkit components: */
  const char *opt_path = "../third-party/senna/";
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
  tokenizer = SENNA_Tokenizer_new(word_hash, caps_hash, suff_hash, gazt_hash, gazl_hash, gazm_hash, gazo_hash, gazp_hash, 1);
}
void free_tokenizer() {
  //clean up senna stuff
  SENNA_Tokenizer_free(tokenizer);

  SENNA_Hash_free(word_hash);
  SENNA_Hash_free(caps_hash);
  SENNA_Hash_free(suff_hash);
  SENNA_Hash_free(gazt_hash);

  SENNA_Hash_free(gazl_hash);
  SENNA_Hash_free(gazm_hash);
  SENNA_Hash_free(gazo_hash);
  SENNA_Hash_free(gazp_hash);
}

void display_ranges(dnmRanges ranges, char* text) {
  int i;
  for (i=0; i<ranges.length; i++) {
    int start = ranges.range[i].start;
    int end = ranges.range[i].end;
    fprintf(stderr,"%d. %.*s\n",i,end-start+1,text+start);
  }
}

dnmRange trim_range(dnmRange range, char* text) {
  while(isspace(text[range.start])) {
    range.start++;
  }
  while(isspace(text[range.end])) {
    range.end--;
  }
  return range;
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

  char* copy = text;
  unsigned int sentence_count = 0;
  unsigned int start_sentence = 0;
  unsigned int end_sentence = 0;
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
        if (has_content) {
          sentence_count++;
          sentence_ranges.range[sentence_count-1].start = start_sentence;
          sentence_ranges.range[sentence_count-1].end = end_sentence;
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
            if (has_content) {
              sentence_count++;
              sentence_ranges.range[sentence_count-1].start = start_sentence;
              sentence_ranges.range[sentence_count-1].end = end_sentence;
            }
            // Init next sentence start:
            copy++;
            start_sentence = ++end_sentence;
            has_content=false;
          }
          else {
            // MathFormula match, so don't break the sentence and move cursor to after math (if we have content)
            copy = copy + ovector[1];
            end_sentence += ovector[1];
            if ((! has_content) || (isupper(*copy))) {
              // But if we have no content (stand-alone formula),
              // OR if the next letter is capital (math sentence terminator),
              //  make a sentence break and save:
              sentence_count++;
              sentence_ranges.range[sentence_count-1].start = start_sentence;
              sentence_ranges.range[sentence_count-1].end = end_sentence-1;
              // Init next sentence start:
              start_sentence = end_sentence;
              has_content=false;
            }
          }
        } else {
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
          has_content = true; }
        copy++;
        end_sentence++;
    } }
  sentence_ranges.length = sentence_count;
  // Trim all sentences:
  int index;
  for (index=0; index<sentence_ranges.length; index++) {
    sentence_ranges.range[index] = trim_range(sentence_ranges.range[index], text);
  }
  //Developer demo:
  //   display_ranges(sentence_ranges, text);
  pcre_free(abbreviation_regex);
  pcre_free(math_formula_regex);
  free_stopwords(); // Release the stopwords
  return sentence_ranges;
}

dnmRanges tokenize_words(dnmRange sentence_range, char* text) {
  int start = sentence_range.start;
  int end = sentence_range.end;
  char* sentence = malloc(sizeof(char) * (end-start+2));
  memcpy( sentence, &text[start], (end-start+1) );
  sentence[(end-start+1)] = '\0';

  // Tokenize with SENNA:
  if (tokenizer == NULL) { initialize_tokenizer(); }
  SENNA_Tokens* tokens = SENNA_Tokenizer_tokenize(tokenizer, sentence);

  dnmRanges word_ranges;
  word_ranges.length = tokens->n;
  word_ranges.range = malloc(tokens->n * sizeof(dnmRange));
  int token_index;
  for(token_index = 0; token_index < tokens->n; token_index++) {
    word_ranges.range[token_index].start = start + tokens->start_offset[token_index];
    word_ranges.range[token_index].end = start + tokens->end_offset[token_index];
    word_ranges.range[token_index] = trim_range(word_ranges.range[token_index], text);
  }
  return word_ranges;
}
