#include <stdio.h>

#define EZLINE_IMPLEMENTATION
#define EZLINE_CTRL_C_ABORT
#include "../ezline.h"

int main(int argc, char **argv) {
  char *line = ezline("What is your name? ");
  if(line == NULL) {
    printf("Goodbye!\n");
    return 1;
  }

  printf("Hello there, %s!\n", line);
  return 0;
}
