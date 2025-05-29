#include <stdio.h>
#include <string.h>
#define EZLINE_IMPLEMENTATION
#include "../ezline.h"

int main(int argc, char **argv) {
  char *line;
  while((line = ezline("> ")) != NULL) {
    if(strcmp(line, "exit") == 0) break;
  }
  return 0;
}
