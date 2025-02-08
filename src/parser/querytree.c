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