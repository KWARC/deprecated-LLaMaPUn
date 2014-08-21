#include <stdio.h>
#include <stdlib.h>

#include "../llamapun_tokenizer.h"
#include <libxml/parser.h>
#include <libxml/tree.h>

int main(void) {
        //load example document
        fprintf(stderr, "\n\n Tokenization tests\n\n");
        xmlDocPtr mydoc = xmlReadFile("../../t/documents/1311.0066.xhtml", NULL, XML_PARSE_RECOVER | XML_PARSE_NONET);
        if (mydoc == NULL) { return 1;}
        //Create DNM, with normalized math tags, and ignoring cite tags
        dnmPtr mydnm = createDNM(mydoc, DNM_NORMALIZE_TAGS);
        if (mydnm == NULL) { return 1;}

        dnmRanges sentences = tokenize_sentences(mydnm->plaintext);

        //clean up
        //free(sentences.range);
        freeDNM(mydnm);
        xmlFreeDoc(mydoc);
        xmlCleanupParser();

        return 0;
}
