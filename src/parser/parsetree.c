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

  if (ins->name != NULL) free(ins->name);
}

static void free_selectstmt(SelectStmt* s) {
  if (s == NULL) return;

  if (s->targetList != NULL) {
    free_parselist(s->targetList);
    free(s->targetList);
  }
}

static void free_restarget(ResTarget* r) {
  if (r == NULL) return;

  if (r->name != NULL) free(r->name);
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
    case T_SelectStmt:
      free_selectstmt((SelectStmt*)n);
      break;
    case T_ParseList:
      free_parselist((ParseList*)n);
      break;
    case T_ResTarget:
      free_restarget((ResTarget*)n);
      break;
    default:
      printf("Unknown node type\n");
  }

  free(n);
}

static void print_restarget(ResTarget* r) {
  printf("%s", r->name);
}

static void print_selectstmt(SelectStmt* s) {
  printf("=  Type: Select\n");
  printf("=  Targets:\n");

  if (s->targetList == NULL || s->targetList->length == 0) {
    printf("=    (none)\n");
    return;
  }

  for (int i = 0; i < s->targetList->length; i++) {
    printf("=    ");
    print_restarget((ResTarget*)s->targetList->elements[i].ptr);
    printf("\n");
  }
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
      printf("=  person_id: %d\n", ((InsertStmt*)n)->personId);
      printf("=  name:      %s\n", ((InsertStmt*)n)->name);
      break;
    case T_SelectStmt:
      print_selectstmt((SelectStmt*)n);
    case T_ParseList:
    case T_ResTarget:
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

ParseList* new_parselist(ParseCell li) {
  ParseList* l = malloc(sizeof(ParseList));
  ParseCell* elements = malloc(sizeof(ParseCell));
  elements[0] = li;

  l->type = T_ParseList;
  l->length = 1;
  l->maxLength = 1;
  l->elements = elements;

  return l;
}

static void enlarge_list(ParseList* list) {
  if (list == NULL) {
    printf("ERROR: list is empty!\n");
    exit(EXIT_FAILURE);
  } else {
    if (list->elements == NULL) {
      list->elements = malloc(sizeof(ParseCell));
    } else {
      list->elements = realloc(list->elements, (list->length + 1) * sizeof(ParseCell));
    }

    list->maxLength++;
  }
}

static inline ParseCell* list_last_cell(const ParseList* list) {
	return &list->elements[list->length];
}

#define lcptr(lc)  ((lc)->ptr)
#define llast(l)  lcptr(list_last_cell(l))

ParseList* parselist_append(ParseList* l, void* cell) {
  if (l->length >= l->maxLength) {
    enlarge_list(l);
  }

  llast(l) = cell;

  l->length++;

  return l;
}

void free_parselist(ParseList* l) {
  if (l != NULL) {
    for (int i = 0; i < l->length; i++) {
      if (&(l->elements[i]) != NULL) {
        free_node(l->elements[i].ptr);
      }
    }

    free(l->elements);
  }
}