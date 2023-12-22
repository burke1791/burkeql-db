# The SQL Analyzer

I want to take some time to introduce the SQL analyzer before moving forward with code changes in this section.

In modern DBMS's, every query you send to it passes through a handful of stages before you get your results back. You can often generalize them in five stages:

1. Lexer/parser
1. Analyzer
1. Rewriter
1. Planner
1. Executor

Right now, we are firmly planted in the lexer/parser (commonly called the parser) stage. This is what we're slowly building up with our flex and bison code. Its sole responsibility is to validate the syntax of the SQL query you give to it. The parser doesn't care if you reference tables/columns/etc that don't actually exist, it just cares that you follow the grammar rules of the query language.

As the parser is validating syntax, it is building a parse tree - a type of abstract syntax tree (AST). This is what we're doing with our variety of `Node` data types. When the parser is done, it sends the parse tree to the analyzer where we perform **semantic** analysis. All it means is we check to make sure any tables or columns referenced in the query actually exist in the database, and ensure all comparisons or operations on certain data types are allowed - things like that.

At this point, we can't write a true analyzer because in order to do its job, it relies on a database's system tables. These are internal tables to the database engine that store metadata about tables, columns, etc. that have been created by the user. Since we haven't implemented system tables yet, we'll need to hard-code a dumb analyzer. Fortunately it'll be pretty simple.

`src/main.c`

```diff
+#include <strings.h>

*** other code removed for brevity ***

+static bool analyze_selectstmt(SelectStmt* s) {
+  for (int i = 0; i < s->targetList->length; i++) {
+    ResTarget* r = (ResTarget*)s->targetList->elements[i].ptr;
+    if (!(strcasecmp(r->name, "person_id") == 0 || strcasecmp(r->name, "name") == 0)) return false;
+  }
+  return true;
+}
+
+static bool analyze_node(Node* n) {
+  switch (n->type) {
+    case T_SelectStmt:
+      return analyze_selectstmt((SelectStmt*)n);
+    default:
+      printf("analyze_node() | unhandled node type");
+  }
+
+  return false;
+}
```

We just add a couple functions to our temporary code section in `main.c`. We have a general `analyze_node` function that will call the `analyze_select` function, which returns true or false depending on the results of our "semantic analysis." All we're doing is checking if the column name in each `ResTarget` is either `person_id` or `name`. We use `strcasecmp`, which does a case-insensitive string comparison.

`src/main.c`

```diff
    switch (n->type) {
      case T_SysCmd:
        if (strcmp(((SysCmd*)n)->cmd, "quit") == 0) {
          free_node(n);
          printf("Shutting down...\n");
          flush_page(fdesc->fd, pg);
          free_page(pg);
          file_close(fdesc);
          return EXIT_SUCCESS;
        }
        break;
      case T_InsertStmt:
        int32_t person_id = ((InsertStmt*)n)->personId;
        char* name = ((InsertStmt*)n)->name;
        if (!insert_record(pg, person_id, name)) {
          printf("Unable to insert record\n");
        }
+       break;
+     case T_SelectStmt:
+       if (!analyze_node(n)) {
+         printf("Semantic analysis failed\n");
+       }
+       break;
    }
```

And in our `main` function we need to make a call to the analyzer. If analysis fails, we simply print a very vague and unhelpful error message - hey that's exactly what all the other SQL parsers/analyzers do too!

## Running the Program

```shell
$ make && ./burkeql
======   BurkeQL Config   ======
= DATA_FILE: /home/burke/source_control/burkeql-db/db_files/main.dbd
= PAGE_SIZE: 128
bql > select person_id, name;
======  Node  ======
=  Type: Select
=  Targets:
=    person_id
=    name
bql > select foo, bar, baz;
======  Node  ======
=  Type: Select
=  Targets:
=    foo
=    bar
=    baz
Semantic analysis failed
bql >
```

Here I tried two select statements. The first one is valid, selecting both of the column names in our hard-coded table. The second gets rejected because none of my columns exist in the table.