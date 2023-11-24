#include <stdio.h>

#include "gram.tab.h"
#include "scan.lex.h"
#include "parser/parsetree.h"

Node* parse_sql() {
  Node* n;
  yyscan_t scanner;

  if (yylex_init(&scanner) != 0) {
    printf("scan init failed\n");
    return NULL;
  }

  int e = yyparse(&n, scanner);

  printf("parse code: %d\n", e);
  yylex_destroy(scanner);

  if (e != 0) printf("Parse error\n");

  return n;
}