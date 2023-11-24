#include <stdio.h>
#include <stdlib.h>

#include "gram.tab.h"

static void print_prompt() {
  printf("bql > ");
}

int main(int argc, char** argv) {
  print_prompt();

  while (!yyparse()) {
    print_prompt();
  }

  return EXIT_SUCCESS;
}