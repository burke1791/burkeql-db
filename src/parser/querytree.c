#include "parser/querytree.h"

Query* new_querytree() {
  Query* qt = create_node(Query);
  return qt;
}

void free_querytree(Query* qt) {
  printf("free_querytree: TBD\n");

  free_parselist(qt->targetList);
  free_parselist(qt->targetList);

  free(qt);
}