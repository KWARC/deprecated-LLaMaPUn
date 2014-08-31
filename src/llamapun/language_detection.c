#include <stdio.h>
#include <stdlib.h>
#include <string.h>


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
/*  DOESN'T WORK YET!!!   */
{
  textcat_t *h;
  char line[1024];
  char *config = "\n\nLM/english.lm\t\t\tenglish\n\
LM/afrikaans.lm\t\t\tafrikaans\n\
LM/albanian.lm\t\t\talbanian\n\
LM/amharic-utf.lm\t\tamharic-utf\n\
LM/arabic-iso8859_6.lm\t\tarabic-iso8859_6\n\
LM/arabic-windows1256.lm\tarabic-windows1256\n\
LM/armenian.lm\t\t\tarmenian\n\
LM/basque.lm\t\t\tbasque\n\
LM/belarus-windows1251.lm\tbelarus-windows1251\n\
LM/bosnian.lm\t\t\tbosnian\n\
LM/breton.lm\t\t\tbreton\n\
LM/bulgarian-iso8859_5.lm\tbulgarian-iso8859_5\n\
LM/catalan.lm\t\t\tcatalan\n\
LM/chinese-big5.lm\t\tchinese-big5\n\
LM/chinese-gb2312.lm\t\tchinese-gb2312\n\
LM/croatian-ascii.lm\t\tcroatian-ascii\n\
LM/czech-iso8859_2.lm\t\tczech-iso8859_2\n\
LM/danish.lm\t\t\tdanish\n\
LM/drents.lm\t\t\tdrents   # Dutch dialect\n\
LM/dutch.lm\t\t\tdutch\n\
LM/esperanto.lm\t\t\tesperanto\n\
LM/estonian.lm\t\t\testonian\n\
LM/finnish.lm\t\t\tfinnish\n\
LM/french.lm\t\t\tfrench\n\
LM/frisian.lm\t\t\tfrisian\n\
LM/georgian.lm\t\t\tgeorgian\n\
LM/german.lm\t\t\tgerman\n\
LM/greek-iso8859-7.lm\t\tgreek-iso8859-7\n\
LM/hebrew-iso8859_8.lm\t\thebrew-iso8859_8\n\
LM/hindi.lm\t\t\thindi\n\
LM/hungarian.lm\t\t\thungarian\n\
LM/icelandic.lm\t\t\ticelandic\n\
LM/indonesian.lm\t\tindonesian\n\
LM/irish.lm\t\t\tirish\n\
LM/italian.lm\t\t\titalian\n\
LM/japanese-euc_jp.lm\t\tjapanese-euc_jp\n\
LM/japanese-shift_jis.lm\tjapanese-shift_jis\n\
LM/korean.lm\t\t\tkorean\n\
LM/latin.lm\t\t\tlatin\n\
LM/latvian.lm\t\t\tlatvian\n\
LM/lithuanian.lm\t\tlithuanian\n\
LM/malay.lm\t\t\tmalay\n\
LM/manx.lm\t\t\tmanx\n\
LM/marathi.lm\t\t\tmarathi\n\
LM/middle_frisian.lm\t\tmiddle_frisian\n\
LM/mingo.lm\t\t\tmingo\n\
LM/nepali.lm\t\t\tnepali\n\
LM/norwegian.lm\t\t\tnorwegian\n\
LM/persian.lm\t\t\tpersian\n\
LM/polish.lm\t\t\tpolish\n\
LM/portuguese.lm\t\tportuguese\n\
LM/quechua.lm\t\t\tquechua\n\
LM/romanian.lm\t\t\tromanian\n\
LM/rumantsch.lm\t\t\trumantsch\n\
LM/russian-iso8859_5.lm\t\trussian-iso8859_5\n\
LM/russian-koi8_r.lm\t\trussian-koi8_r\n\
LM/russian-windows1251.lm\trussian-windows1251\n\
LM/sanskrit.lm\t\t\tsanskrit\n\
LM/scots.lm\t\t\tscots\n\
LM/scots_gaelic.lm\t\tscots_gaelic\n\
LM/serbian-ascii.lm\t\tserbian-ascii\n\
LM/slovak-ascii.lm\t\tslovak-ascii\n\
LM/slovak-windows1250.lm\tslovak-windows1250\n\
LM/slovenian-ascii.lm\t\tslovenian-ascii\n\
LM/slovenian-iso8859_2.lm\tslovenian-iso8859_2\n\
LM/spanish.lm\t\t\tspanish\n\
LM/swahili.lm\t\t\tswahili\n\
LM/swedish.lm\t\t\tswedish\n\
LM/tagalog.lm\t\t\ttagalog\n\
LM/tamil.lm\t\t\ttamil\n\
LM/thai.lm\t\t\tthai\n\
LM/turkish.lm\t\t\tturkish\n\
LM/ukrainian-koi8_r.lm\t\tukrainian-koi8_r\n\
LM/vietnamese.lm\t\tvietnamese\n\
LM/welsh.lm\t\t\twelsh\n\
LM/yiddish-utf.lm\t\tyiddish-utf\n\n";

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
    if ( fp_Read( h->fprint[h->size], segment[0], 400 ) == 0 ) {
      textcat_Done(h);
      fprintf(stderr, "e2");
      goto ERROR;
    }   
    h->size++;
  }

  fclose(fp);
  return h;

 ERROR:
  fclose(fp);
  return NULL;

}