#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include <pcre.h>
#include "stopwords.h"
#include "dnmlib.h"

#define abbreviation_pattern "\\W(?:C(?:[ft]|o(?:n[jn]|lo?|rp)?|a(?:l(?:if)?|pt)|mdr|p?l|res)|M(?:[dst]|a(?:[jnry]|ss)|i(?:ch|nn|ss)|o(?:nt)?|ex?|rs?)|A(?:r(?:[ck]|iz)|l(?:t?a)?|ttys?|ssn|dm|pr|ug|ve)|c(?:o(?:rp|l)?|(?:ap)?t|mdr|p?l|res|f)|S(?:e(?:ns?|pt?|c)|(?:up|g)?t|ask|r)|s(?:e(?:ns?|pt?|c)|(?:up|g)?t|r)|a(?:ttys?|ssn|dm|pr|rc|ug|ve|l)|P(?:enna?|-a.s|de?|lz?|rof|a)|D(?:e(?:[cfl]|p?t)|ist|ak|r)|I(?:[as]|n[cd]|da?|.e|ll)|F(?:e[bd]|w?y|ig|la|t)|O(?:k(?:la)?|[cn]t|re)|d(?:e(?:p?t|c)|ist|r)|E(?:xpy?|.g|sp|tc|q)|R(?:e(?:ps?|sp|v)|d)|T(?:e(?:nn|x)|ce|hm)|e(?:xpy?|.g|sp|tc|q)|m(?:[st]|a[jry]|rs?)|r(?:e(?:ps?|sp|v)|d)|N(?:e(?:br?|v)|ov?)|W(?:isc?|ash|yo?)|f(?:w?y|eb|ig|t)|p(?:de?|lz?|rof)|J(?:u[ln]|an|r)|U(?:SAFA|niv|t)|j(?:u[ln]|an|r)|K(?:ans?|en|y)|B(?:lv?d|ros)|b(?:lv?d|ros)|G(?:en|ov|a)|L(?:td?|a)|g(?:en|ov)|i(?:.e|nc)|l(?:td?|a)|[Hh]wa?y|V[ast]|Que|nov?|univ|Yuk|oct|tce|vs)$"
#define math_formula_pattern "^\\s*MathFormula\\s*"
#define max_abbreviation_length 6
#define sentence_termination_delimiters ".?!\n"
#define OVECCOUNT 30    /* should be a multiple of 3 */

void display_sentences(dnmRanges sentence_ranges, char* text) {
    fprintf(stderr, "\n--------------\nSentences: \n");
    int i;
    for (i=0; i<sentence_ranges.length; i++) {
        int start = sentence_ranges.range[i].start;
        int end = sentence_ranges.range[i].end;
        fprintf(stderr,"%d. %.*s\n",i,end-start+1,text+start);
    }
}

dnmRange trim_sentence(dnmRange range, char* text) {
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
            if ( isalnum(*(copy-1)) && (!isalnum(*(copy-2))) && (*(copy-1) != 'I') ) {
                // Don't consider single letters followed by a punctuation sign an end of a sentence, if the next char is not a space/line break.
                // also "a.m." and "p.m." shouldn't get split
                copy++;
                end_sentence++;
                break;
            }
            //TODO: Handle dot-dot-dot "..."
        case '?':
        case '!':
            // Or quote-enclosed punctuation: "." "?" "!"
            if  ((*(copy-1) == '"') && (*(copy+1) == '"')) {
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
        sentence_ranges.range[index] = trim_sentence(sentence_ranges.range[index], text);
    }
    //Developer demo:
    display_sentences(sentence_ranges, text);
    pcre_free(abbreviation_regex);
    pcre_free(math_formula_regex);
    return sentence_ranges;
}
