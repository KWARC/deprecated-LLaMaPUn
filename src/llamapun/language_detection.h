#include <textcat.h>
int text_is_english(void *tc_handle, const char *text, size_t text_size);


void *llamapun_textcat_Init();    //returns a textcat handle (has to be freed by calling textcat_Done)