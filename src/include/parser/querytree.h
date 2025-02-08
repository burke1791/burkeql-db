#ifndef QUERYTREE_H
#define QUERYTREE_H

#include "parser/parsetree.h"
#include "storage/record.h"

/**
 * The querytree is the result of calling semantic analysis on
 * a SQL statement.
 * 
 */

typedef struct ParseList List;

typedef enum StatementType {
  STMT_SELECT,
  STMT_INSERT,
  STMT_UPDATE,
  STMT_DELETE,
  STMT_SYSCMD,
  STMT_ERROR
} StatementType;

typedef struct Query {
  NodeTag type;
  StatementType stmt;
  List* targetList;
  List* tableList;
  char *errorMessage;
} Query;


/**
 * @brief Query tree node representing a table in the from clause.
 * The analyzer will populate it with enough information for the
 * planner to generate a plan tree
 * 
 */
typedef struct TableEntry {
  NodeTag type;
  int32_t tableId;
  char* name;
  Alias* alias;
  List* columns;
} TableEntry;


/**
 * @brief Query tree node representing a column in the select clause. The analyzer will populate
 * it with enough information for the planner to generate a suitable execution plan
 * 
 */
typedef struct TargetEntry {
  NodeTag type;
  int resNum;
  char* name;
  Alias* alias;
  TableRef* sourceTable;
  DataType dataType;
} TargetEntry;


Query* new_querytree();
void free_querytree(Query* qt);

TableEntry *new_tableentry(int32_t tableId, char *name, Alias *alias, List *columns) {
  
}

#endif /* QUERYTREE_H */