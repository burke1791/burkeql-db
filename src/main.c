#include <stdio.h>
#include <stdlib.h>

#include "gram.tab.h"

extern int yylineno;

static void print_prompt() {
  printf("bql > ");
}

int main(int argc, char** argv) {
  print_prompt();

  while (!yyparse()) {
    print_prompt();
    yylineno = 0;
  }

  return EXIT_SUCCESS;
}