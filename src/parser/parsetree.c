#include <stdlib.h>
#include <stdio.h>

#include "parser/parsetree.h"

static void free_syscmd(SysCmd* sc) {
  if (sc == NULL) return;

  free(sc->cmd);
}

/**
 * @brief 
 * 
 * @param n 
 */
void free_node(Node* n) {
  if (n == NULL) return;

  switch (n->type) {
    case T_SysCmd:
      free_syscmd((SysCmd*)n);
      break;
    default:
      printf("Unknown node type\n");
  }

  free(n);
}

void print_node(Node* n) {
  if (n == NULL) {
    printf("print_node() | Node is NULL\n");
    return;
  }

  printf("======  Node  ======\n");

  switch (n->type) {
    case T_SysCmd:
      printf("=  Type: SysCmd\n");
      printf("=  Cmd: %s\n", ((SysCmd*)n)->cmd);
      break;
    default:
      printf("print_node() | unknown node type\n");
  }
}