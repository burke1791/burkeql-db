# Putting It Together

With all of the complex pieces code written, all we need to do is refactor the `main.c` file and update our `Makefile`.

Here's the full diff of `src/main.c`:

```diff
 #include <stdio.h>
 #include <stdlib.h>
+#include <stdbool.h>
+#include <string.h>
 
 #include "gram.tab.h"
+#include "parser/parsetree.h"
+#include "parser/parse.h"
 
 static void print_prompt() {
   printf("bql > ");
 }
 
 int main(int argc, char** argv) {
-  print_prompt();

-  while(!yyparse()) { 
+  while(true) {
+    print_prompt();
+    Node* n = parse_sql();
+
+    if (n == NULL) continue;
+
+    switch (n->type) {
+      case T_SysCmd:
+        if (strcmp(((SysCmd*)n)->cmd, "quit") == 0) {
+          print_node(n);
+          free_node(n);
+          printf("Shutting down...\n");
+          return EXIT_SUCCESS;
+        }
+      default:
+        print_node(n);
+        free_node(n);
+    }
+  }
 
   return EXIT_SUCCESS;
 }
```

We essentially rewrite the main function from scratch. Using an infinite loop, we make a call to the parse function and get an AST in return. If the tree is `NULL`, as is the case for our current `insert_stmt` and `select_stmt` grammar paths, we jump back to the beginning of the loop.

If we receive an actual AST, we check to see if the user provided the `\quit` command. If so, we clean up our allocated memory and exit the program.

## Makefile

Here's the full diff of the inner Makefile (`src/Makefile`):

```diff
 CC = gcc
 LEX = flex
 YACC = bison
-CFLAGS = -fsanitize=address -static-libasan -g
+CFLAGS = -I./ -I./include -fsanitize=address -static-libasan -g
 
 TARGET_EXEC = burkeql
 
 BUILD_DIR = ..

-SRC_FILES = main.c 
+SRC_FILES = main.c \
+			 parser/parse.c \
+			 parser/parsetree.c
 
 
 $(BUILD_DIR)/$(TARGET_EXEC): gram.tab.o lex.yy.o ${SRC_FILES}
 	${CC} ${CFLAGS} -o $@ $?
 
 gram.tab.c gram.tab.h: parser/gram.y
 	${YACC} -vd $?
 
 lex.yy.c: parser/scan.l
 	${LEX} -o $*.c $<
 
 lex.yy.o: gram.tab.h lex.yy.c
 
 clean:
 	rm -f $(wildcard *.o)
 	rm -f $(wildcard *.output)
 	rm -f $(wildcard *.tab.*)
 	rm -f lex.yy.c
+	rm -f $(wildcard *.lex.*)
 	rm -f $(BUILD_DIR)/$(TARGET_EXEC)
```

## Project Structure

And here is how our repo structure looks:

```shell
├── Makefile
└── src
    ├── Makefile
    ├── include
    │   └── parser
    │       ├── parse.h
    │       └── parsetree.h
    ├── main.c
    └── parser
        ├── gram.y
        ├── parse.c
        ├── parsetree.c
        └── scan.l
```

## Running the Program

```shell
$ make
*** There will be several warnings, but you can safely ignore them ***
make[1]: Leaving directory '***'
$ ./burkeql
bql > select
SELECT command received
bql > insert
INSERT command received
bql > \q
======  Node  ======
=  Type: SysCmd
=  Cmd: q
bql > \quit
======  Node  ======
=  Type: SysCmd
=  Cmd: quit
Shutting down...
$ 
```

Our parser still recognizes the select and insert tokens, but we don't do anything with them. It also recognizes a `\` as a system command flag, but `\q` isn't a valid system command so we don't do anything with it. But when we type `\quit`, our code recognizes the system command flag AND knows to actually quit the program.