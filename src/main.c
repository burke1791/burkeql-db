#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "gram.tab.h"
#include "parser/parsetree.h"
#include "parser/parse.h"

static void print_prompt() {
  printf("bql > ");
}

int main(int argc, char** argv) {

  while(true) {
    print_prompt();
    Node* n = parse_sql();

    if (n == NULL) continue;

    switch (n->type) {
      case T_SysCmd:
        if (strcmp(((SysCmd*)n)->cmd, "quit") == 0) {
          print_node(n);
          free_node(n);
          printf("Shutting down...\n");
          return EXIT_SUCCESS;
        }
      default:
        print_node(n);
        free_node(n);
    }
  }

  return EXIT_SUCCESS;
}