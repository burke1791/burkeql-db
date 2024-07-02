#ifndef QUERYTREE_H
#define QUERYTREE_H

#include "parser/parsetree.h"

/**
 * The querytree is the result of calling semantic analysis on
 * a SQL statement.
 * 
 */

typedef struct ParseList List;

typedef struct Query {
  NodeTag type;
  List* targetList;
} Query;

#endif /* QUERYTREE_H */