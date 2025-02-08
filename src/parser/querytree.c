#include <stdlib.h>
#include <stdio.h>

#include "parser/querytree.h"

Query* new_querytree() {
  Query* qt = create_node(Query);
  qt->targetList = NULL;
  qt->tableList = NULL;
  return qt;
}

void free_querytree(Query* qt) {
  printf("free_querytree: TBD\n");

  free_parselist(qt->targetList);
  free_parselist(qt->targetList);

  free(qt);
}

TableEntry *new_tableentry(int32_t tableId, char *name, Alias *alias) {
  TableEntry *t = malloc(sizeof(TableEntry));
  t->type = T_TableEntry;
  t->tableId = tableId;
  t->name = 
}