# Parser Interface

Since interacting with our parser is going to get more complicated, I am going to pull some of that code out of our main function. We'll define a very basic `parse.h` header:

```c
#include "parsetree.h"

Node* parse_sql();
```

Very simple. We just expose a function that returns the root `Node` of an AST.

And we define the function as follows:

```c
Node* parse_sql() {
  Node* n;
  yyscan_t scanner;

  if (yylex_init(&scanner) != 0) {
    printf("scan init failed\n");
    return NULL;
  }

  int e = yyparse(&n, scanner);

  yylex_destroy(scanner);

  if (e != 0) printf("Parse error\n");

  return n;
}
```

First we have to initialize the scanner, something we didn't have to do before. This is why we needed flex to generate a header file for us - we need access to its structs and functions. Then we call `yyparse` just like last time; however, now we're able to send it our `Node*` struct so that it can build an AST for us. Finally, since we initialized a scanner, we also have to destroy it. Then we can return the AST to the caller.

## Full Files

`src/include/parser/parse.h`

```c
#ifndef PARSE_H
#define PARSE_H

#include "parsetree.h"

Node* parse_sql();

#endif /* PARSE_H */
```

`src/parser/parse.c`

```c
#include <stdio.h>

#include "gram.tab.h"
#include "scan.lex.h"
#include "parser/parsetree.h"

Node* parse_sql() {
  Node* n;
  yyscan_t scanner;

  if (yylex_init(&scanner) != 0) {
    printf("scan init failed\n");
    return NULL;
  }

  int e = yyparse(&n, scanner);

  yylex_destroy(scanner);

  if (e != 0) printf("Parse error\n");

  return n;
}
```