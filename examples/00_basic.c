#include <stdio.h>

#define EZLINE_IMPLEMENTATION
#define EZLINE_CTRL_C_ABORT
#include "../ezline.h"

int main(int argc, char **argv) {
  char *line;
  do {
    line = ezline("What is your name? ");
  } while(line == NULL);

  if(ezline_stat() == EZLINE_STAT_ABORT) {
    printf("Goodbye!\n");
    return 1;
  }

  printf("Hello there, %s!\n", line);
  return 0;
}
