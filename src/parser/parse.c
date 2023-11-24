#include <stdio.h>

#include "gram.tab.h"
#include "parser/parsetree.h"

Node* parse_sql() {
  Node* n;

  int e = yyparse(&n);

  printf("parse code: %d\n", e);

  if (e != 0) printf("Parse error\n");

  return n;
}