#include <stdio.h>
#include <string.h>
#define EZLINE_IMPLEMENTATION
#include "../ezline.h"

int main(int argc, char **argv) {
  char *line = ezline("What is your name? ");
  if(line == NULL) {
    fprintf(stderr, "No username was provided!");
    return 1;
  }
  printf("Your name is: %s\n", line);
  return 0;
}
