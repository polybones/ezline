#include <stdio.h>

#define EZLINE_IMPLEMENTATION
#include "../ezline.h"

int main(int argc, char **argv) {
  char *line;
  do {
    line = ezline("What is your name? ");
  } while(ezline_stat() == EZLINE_STAT_CANCEL);

  if(ezline_stat() != EZLINE_STAT_OK) {
    printf("Goodbye!\n");
    return 1;
  }
  printf("Hello there, %s!\n", line);
  return 0;
}
