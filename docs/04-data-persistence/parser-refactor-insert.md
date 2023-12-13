# Parser Refactor - Insert

In order to support insert operations, we need to update our lexer/parser to identify when we want to insert data.

Fair warning, the next several "chapters" of this guide are going to involve writing a lot of code we're going to throw out in future sections. I'm doing it this way because it allows me to focus on a specific topic while keeping the necessary code footprint small. As we go through the next few chapters, we'll slowly build our permanent codebase, only using the temporary code as stepping stones towards a more robust database program.

You might be wondering how we're going to insert data into a table we haven't created yet. Simple, we're going to hard-code a basic table definition into our program - this is the first of the throwaway code we'll be writing. Our table can be defined by the following DDL:

```sql
Create Table person (
    person_id Int Not Null,
    name Char(20) Not Null
);
```

So we need to update the lexer and parser to identify when we want to insert data AND what the values that we pass to it are. Our grammar isn't going to look like SQL just yet. We're going to define an insert statement as such:

```shell
bql > insert [person_id] [name]
```

Where `person_id` is a raw number and `name` is a QUOTED string (single quotes). So if I wanted to insert person_id: 5, name: Chris Burke, I would write:

```shell
bql > insert 5 'Chris Burke'
```

## Parsetree

`src/include/parser/parsetree.h`

```diff
 typedef enum NodeTag {
-  T_SysCmd
+  T_SysCmd,
+  T_InsertStmt
 } NodeTag;

 typedef struct Node {
   NodeTag type;
 } Node;
 
 typedef struct SysCmd {
   NodeTag type;
   char* cmd;
 } SysCmd;
 
+typedef struct InsertStmt {
+  NodeTag type;
+  int personId;
+  char* firstName;
+} InsertStmt;
```

In our parsetree header we're just adding a new node type and struct to support our new insert statement. Here, you can see we're beginning to hard-code our table definition.

`src/parser/parsetree.c`

```diff
 #include <stdlib.h>
+#include <string.h>
 #include <stdio.h>
 
 #include "parser/parsetree.h"
 
 static void free_syscmd(SysCmd* sc) {
   if (sc == NULL) return;
 
   free(sc->cmd);
 }
 
+static void free_insert_stmt(InsertStmt* ins) {
+  if (ins == NULL) return;
+
+  if (ins->firstName != NULL) free(ins->firstName);
+}
+
 void free_node(Node* n) {
   if (n == NULL) return;
 
   switch (n->type) {
     case T_SysCmd:
       free_syscmd((SysCmd*)n);
       break;
+    case T_InsertStmt:
+      free_insert_stmt((InsertStmt*)n);
+      break;
     default:
       printf("Unknown node type\n");
   }
 
   free(n);
 }
```

In our parsetree implementation, we need to write a function that can free the new `InsertStmt` struct. Remember, we don't need a special function to allocate memory for it because we wrote that `create_node` macro in the parsetree header.

```diff
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
+    case T_InsertStmt:
+      printf("=  Type: Insert\n");
+      printf("=  person_id:  %d\n", ((InsertStmt*)n)->personId);
+      printf("=  first_name: %s\n", ((InsertStmt*)n)->firstName);
+      break;
     default:
       printf("print_node() | unknown node type\n");
   }
 }
```

I'm also adding the insert node to our `print_node` logic just so we can make sure the parser processes everything correctly.

```diff
+char* str_strip_quotes(char* str) {
+  int length = strlen(str);
+  char* finalStr = malloc(length - 1);
+  memcpy(finalStr, str + 1, length - 2);
+  finalStr[length - 2] = '\0';
+  free(str);
+  return finalStr;
+}
```

Lastly, we need a helper function to strip the single quote characters off of the matched input from the lexer. When we write `insert 5 'chris burke'`, our lexer will match all of `'chris burke'`, including the quote characters. We don't want to store those in the table, so we need a way of stripping them out.

## Lexer

`src/parser/scan.l`

```diff
 SELECT    { return SELECT; }
 
+  /* numbers */
+-?[0-9]+    { yylval->intval = atoi(yytext); return INTNUM; }
+
+  /* strings */
+'(\\.|[^'\n])*'  { yylval->str = strdup(yytext); return STRING; }
+
   /* everything else */
 [ \t\n]   /* whitespace */
```

We need to write some additional regex patterns to match integers and quoted strings. If you understand regex, these should make sense to you. This string pattern might be a little confusing, but essentially it will match anything surrounded by single quotes except for line terminators and a single quote character itself. We'll worry about allowing escaped single quotes later.

We're also adding two new tokens: `INTNUM` and `STRING` - defined in the grammar file. When we match these, we need to provide a value to send along to bison. That's where the `yylval->...` assignment statement comes from. The `intval` and `str` properties are defined in bison's `%union` section and their values will be made available to whatever code parses the associated token.

## Grammar

`src/parser/gram.y`

```diff
 %union {
   char* str;
+  int intval;
 
   struct Node* node;
 }
 
 %parse-param { struct Node** n }
 %param { void* scanner }
 
+%token <str> SYS_CMD STRING
+
+%token <intval> INTNUM
 
 /* reserved keywords in alphabetical order */
 %token INSERT
```

Here we add the `intval` property to our union, which gives flex access to it via `yylval->intval`. And we also add our new tokens. The difference between `%token <type_name> TOKEN_NAME` and `%token TOKEN_NAME` is just that those with an associated `<type_name>` will have a value attached to them.

```diff
+insert_stmt: INSERT INTNUM STRING  {
-      printf("INSERT command received\n");
-      $$ = NULL;
+      InsertStmt* ins = create_node(InsertStmt);
+      ins->personId = $2;
+      ins->firstName = str_strip_quotes($3);
+      $$ = (Node*)ins;
     }
   ;
```

## Running the Program

Since we didn't add any new files, we don't need to add anything to the Makefile. We can just compile and run:

```shell
$ make && ./burkeql
======   BurkeQL Config   ======
= DATA_FILE: /home/burke/source_control/burkeql-db/db_files/main.dbd
= PAGE_SIZE: 128
bql > insert 5 'chris burke'
======  Node  ======
=  Type: Insert
=  person_id:  5
=  first_name: chris burke
bql >
```

As expected, our parser was able to identify both the Int and string we want to insert, as well as successfully strip the single quote characters from the string.