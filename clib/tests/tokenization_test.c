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
        old_dnmPtr mydnm = old_createDNM(mydoc, DNM_NORMALIZE_MATH | DNM_SKIP_CITE | DNM_EXTEND_PARA_RANGE);
        if (mydnm == NULL) { return 1;}

        old_dnm_offset* sentences = tokenize_sentences(mydnm->plaintext);

        //clean up
        old_freeDNM(mydnm);
        xmlFreeDoc(mydoc);
        xmlCleanupParser();

        return 0;
}
