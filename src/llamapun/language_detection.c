#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "file_paths.h"

//a bit hacked, but it works:
#include <textcat.c>
//Reason: Datatypes required for llamapun_textcat_Init are declared in .c file


int text_is_english(void *tc, const char *text, size_t text_size) {
  if (tc == NULL) {
    fprintf(stderr, "textcat handle is NULL\n");
    return 0;
  }

  char *result = textcat_Classify(tc, text, text_size /*> 5000 ? 5000 : dnm->size_plaintext*/ );

  if (strncmp(result, "[english]", strlen("[english]"))) {  //isn't primarily english
    return 0;
  }
  
  //otherwise
  return 1;
}

void *llamapun_textcat_Init()
/*  the code from the textcat library, slightly adapted, to avoid external config files. */
{
  textcat_t *h;
  char line[1024];
  char *config = "\n\nenglish.lm\t\t\tenglish\n\
afrikaans.lm\t\t\tafrikaans\n\
albanian.lm\t\t\talbanian\n\
amharic-utf.lm\t\tamharic-utf\n\
arabic-iso8859_6.lm\t\tarabic-iso8859_6\n\
arabic-windows1256.lm\tarabic-windows1256\n\
armenian.lm\t\t\tarmenian\n\
basque.lm\t\t\tbasque\n\
belarus-windows1251.lm\tbelarus-windows1251\n\
bosnian.lm\t\t\tbosnian\n\
breton.lm\t\t\tbreton\n\
bulgarian-iso8859_5.lm\tbulgarian-iso8859_5\n\
catalan.lm\t\t\tcatalan\n\
chinese-big5.lm\t\tchinese-big5\n\
chinese-gb2312.lm\t\tchinese-gb2312\n\
croatian-ascii.lm\t\tcroatian-ascii\n\
czech-iso8859_2.lm\t\tczech-iso8859_2\n\
danish.lm\t\t\tdanish\n\
drents.lm\t\t\tdrents   # Dutch dialect\n\
dutch.lm\t\t\tdutch\n\
esperanto.lm\t\t\tesperanto\n\
estonian.lm\t\t\testonian\n\
finnish.lm\t\t\tfinnish\n\
french.lm\t\t\tfrench\n\
frisian.lm\t\t\tfrisian\n\
georgian.lm\t\t\tgeorgian\n\
german.lm\t\t\tgerman\n\
greek-iso8859-7.lm\t\tgreek-iso8859-7\n\
hebrew-iso8859_8.lm\t\thebrew-iso8859_8\n\
hindi.lm\t\t\thindi\n\
hungarian.lm\t\t\thungarian\n\
icelandic.lm\t\t\ticelandic\n\
indonesian.lm\t\tindonesian\n\
irish.lm\t\t\tirish\n\
italian.lm\t\t\titalian\n\
japanese-euc_jp.lm\t\tjapanese-euc_jp\n\
japanese-shift_jis.lm\tjapanese-shift_jis\n\
korean.lm\t\t\tkorean\n\
latin.lm\t\t\tlatin\n\
latvian.lm\t\t\tlatvian\n\
lithuanian.lm\t\tlithuanian\n\
malay.lm\t\t\tmalay\n\
manx.lm\t\t\tmanx\n\
marathi.lm\t\t\tmarathi\n\
middle_frisian.lm\t\tmiddle_frisian\n\
mingo.lm\t\t\tmingo\n\
nepali.lm\t\t\tnepali\n\
norwegian.lm\t\t\tnorwegian\n\
persian.lm\t\t\tpersian\n\
polish.lm\t\t\tpolish\n\
portuguese.lm\t\tportuguese\n\
quechua.lm\t\t\tquechua\n\
romanian.lm\t\t\tromanian\n\
rumantsch.lm\t\t\trumantsch\n\
russian-iso8859_5.lm\t\trussian-iso8859_5\n\
russian-koi8_r.lm\t\trussian-koi8_r\n\
russian-windows1251.lm\trussian-windows1251\n\
sanskrit.lm\t\t\tsanskrit\n\
scots.lm\t\t\tscots\n\
scots_gaelic.lm\t\tscots_gaelic\n\
serbian-ascii.lm\t\tserbian-ascii\n\
slovak-ascii.lm\t\tslovak-ascii\n\
slovak-windows1250.lm\tslovak-windows1250\n\
slovenian-ascii.lm\t\tslovenian-ascii\n\
slovenian-iso8859_2.lm\tslovenian-iso8859_2\n\
spanish.lm\t\t\tspanish\n\
swahili.lm\t\t\tswahili\n\
swedish.lm\t\t\tswedish\n\
tagalog.lm\t\t\ttagalog\n\
tamil.lm\t\t\ttamil\n\
thai.lm\t\t\tthai\n\
turkish.lm\t\t\tturkish\n\
ukrainian-koi8_r.lm\t\tukrainian-koi8_r\n\
vietnamese.lm\t\tvietnamese\n\
welsh.lm\t\t\twelsh\n\
yiddish-utf.lm\t\tyiddish-utf\n\n";

  FILE *fp = fmemopen(config, strlen(config), "r");

  h = (textcat_t *)wg_malloc(sizeof(textcat_t));
  h->size = 0;
  h->maxsize = 16;
  h->fprint = (void **)wg_malloc( sizeof(void*) * h->maxsize );

  while ( wg_getline( line, 1024, fp ) ) {
    char *p;
    char *segment[4];
    int res;

    /*** Skip comments ***/
#ifdef HAVE_STRCHR
    if (( p = strchr(line,'#') )) {
#else
    if (( p = index(line,'#') )) {
#endif

      *p = '\0';
    }
    if ((res = wg_split( segment, line, line, 4)) < 2 ) {
      continue;
    }

    /*** Ensure enough space ***/
    if ( h->size == h->maxsize ) {
      h->maxsize *= 2;
      h->fprint = (void *)wg_realloc( h->fprint, sizeof(void*) * h->maxsize );
    }

    /*** Load data ***/
    if ((h->fprint[ h->size ] = fp_Init( segment[1] ))==NULL) {
      goto ERROR;
    }
    char *newpath;
    size_t tmp;
    FILE *tmpstream = open_memstream(&newpath, &tmp);
    fprintf(tmpstream, "%s%s", LIBTEXT_LANGUAGES_PATH, segment[0]);
    fclose(tmpstream);
    if ( fp_Read( h->fprint[h->size], newpath, 400 ) == 0 ) {
      free(newpath);
      textcat_Done(h);
      goto ERROR;
    }
    free(newpath);
    h->size++;
  }

  fclose(fp);
  return h;

 ERROR:
 
  fclose(fp);
  return NULL;

}