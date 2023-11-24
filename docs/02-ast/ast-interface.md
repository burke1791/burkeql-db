# AST Interface

The first thing we need to do is begin writing the header file for our AST. In it we'll define all structs available to the parser, as well as some helper functions for creating and destroying nodes.

## The Node

The most basic unit of our AST is the `Node` struct - every single node in our tree is a `Node`. A `Node` can be one of many different types that we'll define later, but they're all just the same base unit.

The primary benefit of doing it this way is minimizing the number of tree-walker functions we need to expose to external processes. We only need to expose a single `void process_node(Node* n)` function that is capable of handling everything. If we had separate node types, e.g. `SelectNode`, `FromNode`, `SortNode`, etc. We would need to expose a different function for each of them. Moreover, there's a clever trick we can do with macros that makes creating different node types in the parser very easy.

## Header File

### Structs

First we define our structs:

```c
typedef enum NodeTag {
  T_SysCmd
} NodeTag;

typedef struct Node {
  NodeTag type;
} Node;

typedef struct SysCmd {
  NodeTag type;
  char* cmd;
} SysCmd;
```

The `NodeTag` enum is how we signify the type of node we're dealing with. Next up is the `Node`, our basic AST building block. Containing only a single property, the `Node`'s only purpose is a common junction for jumping between node types. And finally, we introduce the `SysCmd` struct. All `SysCmd` structs will have `type = T_SysCmd`, and a string representing the command the user passed to it. For example, the quit command (`\q`) will produce a `SysCmd` struct with `cmd = "q"`.

### Macros

Next we define a couple macros that well help us in the bison grammar for easily creating nodes. Though the "easily" part may not become apparent until we begin supporting a lot more node types.

```c
#define new_node(size, tag) \
({ Node* _result; \
  _result = (Node*)malloc(size); \
  _result->type = (tag); \
  _result; \
})

#define create_node(_type_)   ((_type_*) new_node(sizeof(_type_), T_##_type_))
```

When the parser encounters a system command, the grammar rule will have it run:

```c
SysCmd* sc = create_node(SysCmd);
```

Using macro expansion, it's able to allocate the correct amount of memory for the `SysCmd` node type and set the `type` property correctly. The `create_node` macro is essentially shorthand for: 

```c
SysCmd* sc = malloc(sizeof(SysCmd));
sc->type = T_SysCmd;
```

### API

Finally, we want to define the interface functions to our AST:

```c
void free_node(Node* n);

void print_node(Node* n);
```

At the moment, we only need two functions - one for cleanup and one for printing/debugging. Again, because we use the `Node` as our basic building block, we're only exposing `free_node` and `print_node` instead of `free_syscmd` and `print_syscmd` (plus any other future node types).

## Full File

`src/include/parser/parsetree.h`

```c
#ifndef PARSETREE_H
#define PARSETREE_H

typedef enum NodeTag {
  T_SysCmd
} NodeTag;

typedef struct Node {
  NodeTag type;
} Node;

typedef struct SysCmd {
  NodeTag type;
  char* cmd;
} SysCmd;

#define new_node(size, tag) \
({ Node* _result; \
  _result = (Node*)malloc(size); \
  _result->type = (tag); \
  _result; \
})

#define create_node(_type_)   ((_type_*) new_node(sizeof(_type_), T_##_type_))

void free_node(Node* n);

void print_node(Node* n);

#endif /* PARSETREE_H */
```