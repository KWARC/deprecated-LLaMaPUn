#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "old_dnmlib.h"
#include <pcre.h>

#define abbreviation_pattern "(?:^|\\s)(?:C(?:[ft]|o(?:n[jn]|lo?|rp)?|a(?:l(?:if)?|pt)|mdr|p?l|res)|M(?:[dst]|a(?:[jnry]|ss)|i(?:ch|nn|ss)|o(?:nt)?|ex?|rs?)|A(?:r(?:[ck]|iz)|l(?:t?a)?|ttys?|ssn|dm|pr|ug|ve)|c(?:o(?:rp|l)?|(?:ap)?t|mdr|p?l|res|f)|S(?:e(?:ns?|pt?|c)|(?:up|g)?t|ask|r)|s(?:e(?:ns?|pt?|c)|(?:up|g)?t|r)|a(?:ttys?|ssn|dm|pr|rc|ug|ve|l)|P(?:enna?|-a.s|de?|lz?|rof|a)|D(?:e(?:[cfl]|p?t)|ist|ak|r)|I(?:[as]|n[cd]|da?|.e|ll)|F(?:e[bd]|w?y|ig|la|t)|O(?:k(?:la)?|[cn]t|re)|d(?:e(?:p?t|c)|ist|r)|E(?:xpy?|.g|sp|tc|q)|R(?:e(?:ps?|sp|v)|d)|T(?:e(?:nn|x)|ce|hm)|e(?:xpy?|.g|sp|tc|q)|m(?:[st]|a[jry]|rs?)|r(?:e(?:ps?|sp|v)|d)|N(?:e(?:br?|v)|ov?)|W(?:isc?|ash|yo?)|f(?:w?y|eb|ig|t)|p(?:de?|lz?|rof)|J(?:u[ln]|an|r)|U(?:SAFA|niv|t)|j(?:u[ln]|an|r)|K(?:ans?|en|y)|B(?:lv?d|ros)|b(?:lv?d|ros)|G(?:en|ov|a)|L(?:td?|a)|g(?:en|ov)|i(?:.e|nc)|l(?:td?|a)|[Hh]wa?y|V[ast]|Que|nov?|univ|Yuk|oct|tce|vs)\\.$"
#define math_formula_pattern "^\\s*MathFormula\\s*"
#define max_abbreviation_length 6
#define sentence_termination_delimiters ".?!\n"
#define OVECCOUNT 30    /* should be a multiple of 3 */

void display_sentences(char* text, old_dnm_offsets* sentence_offsets) {
  fprintf(stderr, "Sentences: \n");
  int i = 0;
  while (sentence_offsets != NULL) {
    i++;
    int start = sentence_offsets->start;
    int end = sentence_offsets->end;
    sentence_offsets = sentence_offsets->next;
    fprintf(stderr,"%d. %.*s\n",i,end-start+1,text+start);
  }
}

old_dnm_offsets* tokenize_sentences(char* text) {
  const char* error;
  int error_offset;
  int ovector[OVECCOUNT];
  pcre *abbreviation_regex, *math_formula_regex;
  abbreviation_regex = pcre_compile(abbreviation_pattern, 0, &error, &error_offset, NULL);
  math_formula_regex = pcre_compile(math_formula_pattern, 0, &error, &error_offset, NULL);
  old_dnm_offsets* sentence_offsets = malloc(sizeof(old_dnm_offsets));
  old_dnm_offsets* current_offset = sentence_offsets;

  char* copy = text;
  unsigned int sentence_count = 0;
  unsigned int start_sentence = 0;
  unsigned int end_sentence = 0;
  bool line_break_prior = false;
  while ((copy != NULL) && ((*copy) != '\0')) {
    switch (*copy) {
      case '.': // Check if abbreviation:
        line_break_prior = false;
      case '?':
      case '!':
        line_break_prior = false;
        sentence_count++;
        // Save sentence
        current_offset->start = start_sentence;
        current_offset->end = end_sentence;
        current_offset->next = malloc(sizeof(old_dnm_offsets));
        current_offset = current_offset->next;
        // Init next sentence start:
        copy++;
        start_sentence = ++end_sentence;
        // End of sentence punctuation case
        break;
      case '\n': // Multiple new lines are a paragraph break, UNLESS MathFormula is in between
        if (line_break_prior) {
          line_break_prior = false;
          // Look ahead for MathFormula, end sentence if not found
          int rc = pcre_exec(math_formula_regex, NULL, copy, strlen(copy), 0, 0, ovector, OVECCOUNT);
          if (rc < 0) {
            // No match, sentence break here
            // All follow-up whitespace should be taken as part of this sentence:
            while ((*(copy+1) == '\n') || (*(copy+1) == ' ')) {
              copy++;
              end_sentence++;
            }
            // Save sentence
            current_offset->start = start_sentence;
            current_offset->end = end_sentence;
            current_offset->next = malloc(sizeof(old_dnm_offsets));
            current_offset = current_offset->next;
            // Init next sentence start:
            copy++;
            start_sentence = ++end_sentence;
          }
          else {
            // Match, so don't break the sentence and move cursor to after match
            copy = copy + ovector[1];
            end_sentence += ovector[1];
            // Save sentence
            current_offset->start = start_sentence;
            current_offset->end = end_sentence;
            current_offset->next = malloc(sizeof(old_dnm_offsets));
            current_offset = current_offset->next;
            // Init next sentence start:
            copy++;
            start_sentence = ++end_sentence;
          }
        } else { // Mark we've seen a first line break
          line_break_prior = true; }
        // End of line break Case
        break;
      default: //just continue if a regular character
        line_break_prior = false;
        copy++;
        end_sentence++;
    } }

  //Developer demo:
  //   display_sentences(text, sentence_offsets);
  return sentence_offsets;
 }
