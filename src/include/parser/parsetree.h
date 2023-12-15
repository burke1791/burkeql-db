#ifndef PARSETREE_H
#define PARSETREE_H

/**
 * This file defines the interface for working with Abstract Syntax
 * Trees (AST) in BurkeQL.
 * 
 * The basic unit of our AST is the Node struct. All ASTs produced
 * by any process will have a single Node at the root level and
 * many more Node children. A Node only has a single property, the
 * NodeTag enum, whose purpose is to tell the programmer which type of
 * Node the pointer can be cast to.
 */

typedef enum NodeTag {
  T_SysCmd,
  T_InsertStmt
} NodeTag;

typedef struct Node {
  NodeTag type;
} Node;

typedef struct SysCmd {
  NodeTag type;
  char* cmd;
} SysCmd;

typedef struct InsertStmt {
  NodeTag type;
  int personId;
  char* name;
} InsertStmt;


/**
 * These macros, `new_node` and `create_node`, allow us to generically
 * create different new nodes from the bison parser.
 */
#define new_node(size, tag) \
({ Node* _result; \
  _result = (Node*)malloc(size); \
  _result->type = (tag); \
  _result; \
})

#define create_node(_type_)   ((_type_*) new_node(sizeof(_type_), T_##_type_))

void free_node(Node* n);

void print_node(Node* n);

char* str_strip_quotes(char* str);

#endif /* PARSETREE_H */