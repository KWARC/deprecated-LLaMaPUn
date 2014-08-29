#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <uthash.h>
//#include <json-c/json.h>
#include "jsoninclude.h"

#include "stopwords.h"


struct stopword_element *STOPWORDS = NULL;

struct stopword_element {
  char *word;
  UT_hash_handle hh;
};


void load_stopwords() {
  //loads our math stopwords
  const char math_stopwords[676][16] = {"a","able","about","above","abroad","according","accordingly","across","actually","adj","after","afterwards","again","against","ago","ahead","ain't","al","all","allow","allows","almost","alone","along","alongside","already","also","although","always","am","amid","amidst","among","amongst","an","and","another","any","anybody","anyhow","anyone","anything","anyway","anyways","anywhere","apart","appear","appreciate","appropriate","arbitrary","are","aren't","around","a's","as","aside","ask","asking","associated","at","available","away","awfully","b","backward","backwards","be","became","because","become","becomes","becoming","been","before","beforehand","begin","behind","being","believe","below","beside","besides","best","better","between","beyond","both","brief","but","by","c","call","called","came","can","cannot","can't","cant","caption","cause","causes","certain","certainly","case","changes","clearly","c'mon","co.","co","com","come","comes","concerning","consequently","consider","considering","consist","consisting","contain","containing","contains","corresponding","could","couldn't","course","c's","currently","d","dare","daren't","definitely","defined","denote","denoted","described","despite","did","didn't","different","directly","do","does","doesn't","doing","done","don't","down","downwards","during","e","each","easy","edu","eg","eight","eighty","either","else","elsewhere","ending","enough","entirely","entry","especially","et","etc","ever","evermore","every","everybody","everyone","everything","everywhere","ex","exactly","example","except","expressed","express","f","fairly","far","farther","few","fewer","followed","following","follows","for","forever","former","formerly","forth","forward","found","from","further","furthermore","g","get","gets","getting","give","given","gives","goes","going","gone","got","gotten","greetings","h","had","hadn't","happens","hardly","has","hasn't","have","haven't","having","he","he'd","he'll","hello","help","hence","her","here","hereafter","hereby","herein","here's","hereupon","hers","herself","he's","hi","him","himself","his","hither","hopefully","how","howbeit","however","i","i'd","ie","if","ignored","i'll","i'm","immediate","in","inasmuch","inc.","inc","include","includes","indeed","indicate","indicated","indicates","inside","insofar","instead","into","inward","is","isn't","it","it'd","it'll","it's","its","itself","i've","j","just","k","keep","keeps","kept","know","known","knows","l","last","lately","later","latter","latterly","least","less","lest","let","let's","like","liked","likely","likewise","look","looking","looks","ltd","m","made","mainly","make","makes","many","may","maybe","mayn't","me","meantime","meanwhile","merely","might","mightn't","mine","miss","more","moreover","most","mostly","mr","mrs","much","must","mustn't","my","myself","n","name","namely","nd","near","nearly","necessary","need","needn't","needs","neither","never","neverf","neverless","nevertheless","new","next","nine","ninety","no","nobody","non","none","nonetheless","no-one","noone","nor","normally","not","note","notion","nothing","notwithstanding","novel","now","nowhere","o","obtain","obtained","obviously","of","off","often","oh","ok","okay","old","on","once","one","one's","ones","only","onto","opposite","or","originally","other","others","otherwise","ought","oughtn't","our","ours","ourselves","out","outside","over","overall","own","p","particular","particularly","past","per","perhaps","placed","please","possible","presumably","probably","prove","proves","proved","provided","provides","q","que","quite","qv","r","rather","rd","re","really","reasonably","recent","recently","reference","regarding","regardless","regards","relatively","required","respective","respectively","s","said","same","saw","say","saying","says","secondly","see","seeing","seem","seemed","seeming","seems","seen","self","selves","sensible","sent","serious","seriously","seven","several","shall","shan't","she","she'd","she'll","she's","show","shows","showed","should","shouldn't","side","similarly","since","six","so","solve","solving","solved","some","somebody","someday","somehow","someone","something","sometime","sometimes","somewhat","somewhere","soon","sorry","specified","specify","specifying","still","sub","such","sup","suppose","sure","t","take","taken","taking","tell","tends","th","than","thank","thanks","thanx","that","that'll","that's","thats","that've","the","their","theirs","them","themselves","then","thence","there","thereafter","thereby","there'd","therefore","therein","there'll","there're","there's","theres","thereupon","there've","these","they","they'd","they'll","they're","they've","thing","things","think","thirty","this","thorough","thoroughly","those","though","three","through","throughout","thru","thus","till","to","together","too","took","toward","towards","tried","tries","truly","try","trying","t's","twice","two","u","un","under","underneath","undoing","unfortunately","unless","unlike","unlikely","until","unto","up","upon","upwards","us","use","used","useful","uses","using","usually","v","various","versus","very","via","viz","vs","w","want","wants","was","wasn't","way","we","we'd","welcome","we'll","well","went","we're","were","weren't","we've","what","whatever","what'll","what's","what've","when","whence","whenever","where","whereafter","whereas","whereby","wherein","where's","whereupon","wherever","whether","which","whichever","while","whilst","whither","who","who'd","whoever","whole","who'll","whom","whomever","who's","whose","why","will","willing","wish","with","within","without","wonder","won't","work","would","wouldn't","write","written","x","y","yes","yet","you","you'd","you'll","your","you're","yours","yourself","yourselves","you've","z"};
  struct stopword_element *stopword_hash = NULL;
  size_t i;
  for (i=0; i<676; i++) {
    stopword_hash = (struct stopword_element*)malloc(sizeof(struct stopword_element));
    stopword_hash->word = strdup(math_stopwords[i]);
    HASH_ADD_KEYPTR( hh, STOPWORDS, stopword_hash->word, strlen(stopword_hash->word), stopword_hash );
  }
}

void read_stopwords_from_json(json_object *stopwords_json) {
  /* Read in the list of stopwords as a hash */
  
  int stopwords_count = json_object_array_length(stopwords_json);
  struct stopword_element *stopword_hash = NULL;
  int i;
  for (i=0; i< stopwords_count; i++){
    json_object *word_json = json_object_array_get_idx(stopwords_json, i); /*Getting the array element at position i*/
    const char *stopword = json_object_get_string(word_json);
    stopword_hash = (struct stopword_element*)malloc(sizeof(struct stopword_element));
    stopword_hash->word = strdup(stopword);
    HASH_ADD_KEYPTR( hh, STOPWORDS, stopword_hash->word, strlen(stopword_hash->word), stopword_hash );
  }
}

void free_stopwords() {
  /* deallocates the memory */
  struct stopword_element *current, *tmp;
  HASH_ITER(hh, STOPWORDS, current, tmp) {
    HASH_DEL(STOPWORDS,current);
    free(current->word);
    free(current);
  }
}

int is_stopword(const char *word) {
  /* checks whether word is one of the stopwords */
  if (STOPWORDS == NULL) {
    fprintf(stderr, "Stopwords not initialized");
    return 0;
  }
  struct stopword_element* tmp;
  char* lower_cased_word = strdup(word);
  char* p;
  for(p=lower_cased_word ; *p; p++) { *p = tolower(*p); }
  HASH_FIND_STR(STOPWORDS, lower_cased_word, tmp);
  free(lower_cased_word);
  if (tmp == NULL) return 0;
  else return 1;
}
