#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "parser/parsetree.h"

static void free_syscmd(SysCmd* sc) {
  if (sc == NULL) return;

  free(sc->cmd);
}

static void free_insert_stmt(InsertStmt* ins) {
  if (ins == NULL) return;

  if (ins->firstName != NULL) free(ins->firstName);
  if (ins->lastName != NULL) free(ins->lastName);
}

void free_node(Node* n) {
  if (n == NULL) return;

  switch (n->type) {
    case T_SysCmd:
      free_syscmd((SysCmd*)n);
      break;
    case T_InsertStmt:
      free_insert_stmt((InsertStmt*)n);
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
    case T_InsertStmt:
      printf("=  Type: Insert\n");
      printf("=  person_id:  %d\n", ((InsertStmt*)n)->personId);
      printf("=  first_name: %s\n", ((InsertStmt*)n)->firstName);
      printf("=  last_name:  %s\n", ((InsertStmt*)n)->lastName);
      break;
    default:
      printf("print_node() | unknown node type\n");
  }
}

char* str_strip_quotes(char* str) {
  int length = strlen(str);
  char* finalStr = malloc(length - 1);
  memcpy(finalStr, str + 1, length - 2);
  finalStr[length - 2] = '\0';
  free(str);
  return finalStr;
}