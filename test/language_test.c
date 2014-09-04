#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "llamapun/language_detection.h"

int main(void) {
  void *tc_handle = llamapun_textcat_Init();
  if (tc_handle == NULL) {
    fprintf(stderr, "Couldn't load textcat handle\n");
    return 1;
  }

  char *english = "The language detector doesn't detect short sentences properly - so let's take a longer one :)";
  char *latin = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.";

  if (text_is_english(tc_handle, latin, strlen(latin))) {
    fprintf(stderr, "\"%s\" was detected as English - failed!\n", latin);
    textcat_Done(tc_handle);
    return 1;
  }

  if (!text_is_english(tc_handle, english, strlen(english))) {
    //NOTE: This might be caused, because the library wasn't loaded correctly
    fprintf(stderr, "\"%s\" wasn't detected as English - failed!\n", english);
    textcat_Done(tc_handle);
    return 1;
  }

  //everything went well
  textcat_Done(tc_handle);
  return 0;
}