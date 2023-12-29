#ifndef PARSETREE_H
#define PARSETREE_H

#include <stdint.h>

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
  T_InsertStmt,
  T_SelectStmt,
  T_ParseList,
  T_ResTarget
} NodeTag;

typedef struct Node {
  NodeTag type;
} Node;

typedef struct ParseCell {
  void* ptr;
} ParseCell;

typedef struct ParseList {
  NodeTag type;
  int length;
  int maxLength;
  ParseCell* elements;
} ParseList;

typedef struct SysCmd {
  NodeTag type;
  char* cmd;
} SysCmd;

typedef struct InsertStmt {
  NodeTag type;
  int32_t personId;
  char* name;
  uint8_t age;
  int16_t dailySteps;
  int64_t distanceFromHome;
} InsertStmt;

typedef struct ResTarget {
  NodeTag type;
  char* name;
} ResTarget;

typedef struct SelectStmt {
  NodeTag type;
  ParseList* targetList;
} SelectStmt;


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

#define parselist_make_ptr_cell(v)    ((ParseCell) {.ptr = (v)})

#define create_parselist(li)   new_parselist(parselist_make_ptr_cell(li))

void free_node(Node* n);

void print_node(Node* n);

char* str_strip_quotes(char* str);

ParseList* new_parselist(ParseCell li);
void free_parselist(ParseList* l);
ParseList* parselist_append(ParseList* l, void* cell);


#endif /* PARSETREE_H */