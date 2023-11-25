# AST Implementation

With our header file defined, we can start implementing the AST functionality.

First we'll define our `free_node` function. Since it needs to handle all types of nodes, we'll use a switch statement on the `NodeTag` property.

```c
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
```

A forward thinker will see that although we only need to expose a single `free` function, we still need to write specific functions for all different node types. Fortunately, those only need to exist in this file.

```c
static void free_syscmd(SysCmd* sc) {
  if (sc == NULL) return;

  free(sc->cmd);
}
```

We define a static function for the `SysCmd` node that frees memory for all properties within the `SysCmd` node. Note that we do not free the `SysCmd` itself because the outer `free_node` function will do it for us.

Finally, we write the print function:

```c
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
```

This function has a similar structure to the `free_node` function. Later down the road, we will write corresponding `static print_` functions for each node type, but for now the `SysCmd` node is simple enough to live in the switch branch.

## Full File

`src/parser/parsetree.c`

```c
#include <stdlib.h>
#include <stdio.h>

#include "parser/parsetree.h"

static void free_syscmd(SysCmd* sc) {
  if (sc == NULL) return;

  free(sc->cmd);
}

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
```