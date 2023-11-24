#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "gram.tab.h"
#include "parser/parsetree.h"
#include "parser/parse.h"

extern int yylineno;

static void print_prompt() {
  printf("bql > ");
}

int main(int argc, char** argv) {

  while(true) {
    print_prompt();
    Node* n = parse_sql();

    print_node(n);
    free_node(n);
    yylineno = 0;
  }

  return EXIT_SUCCESS;
}