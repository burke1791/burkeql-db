# Parser Refactor - Select

Now that we're able to insert data into our database, we need to add some functionality that can "Select" data back out. We'll start by defining our CLI syntax, then update the lexer/parser to support the new grammar, and finally we'll write the supporting code in our `parsetree` header and code files.

The syntax will be pretty simple:

`bql > select [colname], [colname], ... ;`

We look for the `SELECT` keyword, followed by a comma-separated list of columns we want to select, then ending with a semi-colon. The column names will not be surrounded by quotes - they'll be a class of lexer tokens called `IDENTIFIERS`.

The list of columns can be as long as the user wants, so our parser needs to be smart enough to support a potentially infinite list. And because it can grow forever, we need to introduce a token that tells the parser it's time to stop: the semi-colon.

## Lexer

Starting off simple, the lexer updates are pretty small. We need to add two new match patterns - one for the two punctuation characters, and another for identifiers.

`src/parser/scan.l`

```diff
+  /* operators */
+[,;]   { return yytext[0]; }

   /* strings */
 '(\\.|''|[^'\n])*'  { yylval->str = strdup(yytext); return STRING; }
 
+  /* identifiers */
+[A-Za-z_][A-Za-z0-9_]*   { yylval->str = strdup(yytext); return IDENT; }
```

The comma and semi-colon are very straightforward - just those literal characters. Identifiers are also fairly simple. We want any alphanumeric text without whitespace as long as it doesn't begin with a number.

Easy, right?

## Grammar

Now let's check out the grammar changes.

`src/parser/gram.y`

```diff
 %union {
   char* str;
   int intval;
 
   struct Node* node;
+  struct ParseList* list;
 }
 
 %parse-param { struct Node** n }
 %param { void* scanner }
 
-%token <str> SYS_CMD STRING
+%token <str> SYS_CMD STRING IDENT
 
 %token <intval> INTNUM
 
 /* reserved keywords in alphabetical order */
 %token INSERT
 
 %token SELECT
 
-%type <node> cmd stmt sys_cmd select_stmt insert_stmt
+%type <node> cmd stmt sys_cmd select_stmt insert_stmt target
+ 
+%type <list> target_list
 
 %start query
```

We add a new `ParseList` struct to the union, which will enable us to make lists like the comma-separated list of columns our CLI will accept. We'll cover the details of the struct in the next section. We also need to add a new token for the identifiers our lexer is now able to match.

And closing out the definitions section, we have two new grammar rules: `target` and `target_list`. As you can probably guess, the `target_list` is going to be a `ParseList` to `target`s.

`src/parser/gram.y`

```diff
-cmd: stmt
+cmd: stmt ';'
   | sys_cmd 
   ;
```

At the top, we add the terminator semi-colon to the `cmd` rule when it follows a `stmt`. With this change we'll also need to end our insert statements with a semi-colon in order for the parser to correctly identify them. `sys_cmd`s will not need a semi-colon.

`src/parser/gram.y`

```diff
-select_stmt: SELECT {
+select_stmt: SELECT target_list {
-      printf("SELECT command received\n");
-      $$ = NULL;
+      SelectStmt* s = create_node(SelectStmt);
+      s->targetList = $2;
+      $$ = (Node*)s;
+    }
+  ;
+
+target_list: target {
+      $$ = create_parselist($1);
+    }
+  | target_list ',' target {
+      $$ = parselist_append($1, $3);
+    }
+  ;
+
+target: IDENT {
+      ResTarget* r = create_node(ResTarget);
+      r->name = $1;
+      $$ = (Node*)r;
+    }
+  ;
```

Don't spend too much time worring about new structs/functions you see like `ResTarget` or `create_parselist` - I'll cover those below. Instead pay attention to how the grammar is written.

Here we have refactored the syntax rules for our `select_stmt`. We still begin by looking for the `SELECT` keyword, but now we also require a `target_list`, which is exactly what it sounds like - a list of `targets`. Note the clever grammar definition we use here - a valid `target_list` is either a single `target` or a `target_list` followed by another `target`. This means bison can keep matching `target`s and appending them to the list until the cows come home (or flex sends a semi-colon).

The `target` itself is just an `IDENT` token sent by flex, whose value we stuff into a `ResTarget` node.

## Parsetree Header

These updates to our parser require a fairly significant amount of new code in the `parsetree.c` file, but first let's go over the changes in the header file.

`src/include/parser/parsetree.h`

```diff
 typedef enum NodeTag {
   T_SysCmd,
-  T_InsertStmt
+  T_InsertStmt,
+  T_SelectStmt,
+  T_ParseList,
+  T_ResTarget
 } NodeTag;
```

We add three new node types to our library of nodes. The `SelectStmt`, which we knew was coming, the `ParseList`, and the `ResTarget`. The `ParseList` is just a generic list struct our parser will use to store anything we need to keep in a list. `ResTarget` - kind of short for "Result Target" is a node that stores information about the column our select statement is targeting.

`src/include/parser/parsetree.h`

```diff
 typedef struct Node {
   NodeTag type;
 } Node;
 
+typedef struct ParseCell {
+  void* ptr;
+} ParseCell;
+
+typedef struct ParseList {
+  NodeTag type;
+  int length;
+  int maxLength;
+  ParseCell* elements;
+} ParseList;
+
 typedef struct SysCmd {
   NodeTag type;
   char* cmd;
 } SysCmd;
```

We add structs for the `ParseList`, which keeps an array of `ParseCell`s. Most often, we'll be using the `ParseCell` to store a `Node`, but we define its property to be a generic `void*` so that we can store whatever we want in it.

`src/include/parser/parsetree.h`

```diff
 typedef struct InsertStmt {
   NodeTag type;
   int personId;
   char* name;
 } InsertStmt;
 
+typedef struct ResTarget {
+  NodeTag type;
+  char* name;
+} ResTarget;
+
+typedef struct SelectStmt {
+  NodeTag type;
+  ParseList* targetList;
+} SelectStmt;
```

Next, we define the `ResTarget` and `SelectStmt` nodes. As mentioned above, the `ResTarget` just stores the name of the column we're "select"ing. And the `SelectStmt` stores a list of `ResTarget`s.

`src/include/parser/parsetree.h`

```diff
+#define parselist_make_ptr_cell(v)    ((ParseCell) {.ptr = (v)})
+
+#define create_parselist(li)   new_parselist(parselist_make_ptr_cell(li))
 
 void free_node(Node* n);
 
 void print_node(Node* n);
 
 char* str_strip_quotes(char* str);

+ParseList* new_parselist(ParseCell li);
+void free_parselist(ParseList* l);
+ParseList* parselist_append(ParseList* l, void* cell);
```

Lastly, we have a couple new macros and a few new functions. Similar to the `create_node` macro, I wrote the `create_parselist` macro to simplify list creation. It takes anything as input and first creates a new `ParseList` of length 1, then it creates a `ParseCell` and stores whatever we passed in (`li`) in the `.ptr` field of `ParseCell` and populates the 1-length array with that `ParseCell`.

Next up, we have three common "list" functions: the allocate/free function pair, and a function to append an item to the list.

## Parsetree Code

We have a lot of new code in this file, and it's boring code too, so bear with me.

`src/parser/parsetree.c`

```diff
+ParseList* new_parselist(ParseCell li) {
+  ParseList* l = malloc(sizeof(ParseList));
+  ParseCell* elements = malloc(sizeof(ParseCell));
+  elements[0] = li;
+
+  l->type = T_ParseList;
+  l->length = 1;
+  l->maxLength = 1;
+  l->elements = elements;
+
+  return l;
+}
+
+void free_parselist(ParseList* l) {
+  if (l != NULL) {
+    for (int i = 0; i < l->length; i++) {
+      if (&(l->elements[i]) != NULL) {
+        free_node((Node*)l->elements[i].ptr);
+      }
+    }
+
+    free(l->elements);
+  }
+}
```

Starting with the allocator/free pair of functions. The allocator grabs enough memory for the list struct and a list with a single item - the `ParseCell` input parameter. We populate the first item of the list and set the list metadata fields. Remember, a `ParseList` is also a `Node`, so it needs to have a valid `NodeTag` in its `type` property.

The free function is standard list free logic. We loop through the elements array and call `free_node` on the `.ptr` of each `ParseCell`.

`src/parser/parsetree.c`

```diff
+static inline ParseCell* parselist_last_cell(const ParseList* l) {
+	return &l->elements[l->length];
+}
+
+#define lcptr(lc)  ((lc)->ptr)
+#define llast(l)  lcptr(parselist_last_cell(l))
+
+ParseList* parselist_append(ParseList* l, void* cell) {
+  if (l->length >= l->maxLength) {
+    enlarge_list(l);
+  }
+
+  llast(l) = cell;
+
+  l->length++;
+
+  return l;
+}
+
+static void enlarge_list(ParseList* list) {
+  if (list == NULL) {
+    printf("ERROR: list is empty!\n");
+    exit(EXIT_FAILURE);
+  } else {
+    if (list->elements == NULL) {
+      list->elements = malloc(sizeof(ParseCell));
+    } else {
+      list->elements = realloc(list->elements, (list->length + 1) * sizeof(ParseCell));
+    }
+
+    list->maxLength++;
+  }
+}
```

The `parselist_last_cell` and two macros might look confusing, but it's just an easy way to set the last element in a list.

Our append function grows the list if necessary, then sets the last cell of the list to the `void* cell` input parameter. The `enlarge_list` function will grow the list by 1 each time it's called, using the handy `realloc` function.

Now let's write the free functions for our two new node types:

`src/parser/parsetree.c`

```diff
+static void free_insert_stmt(InsertStmt* ins) {
+  if (ins == NULL) return;
+
+  if (ins->name != NULL) free(ins->name);
+}
+
+static void free_selectstmt(SelectStmt* s) {
+  if (s == NULL) return;
+
+  if (s->targetList != NULL) {
+    free_parselist(s->targetList);
+    free(s->targetList);
+  }
+}
```

Pretty straightforward, simply call `free()` on anything that needs to be free'd.

Next, we'll add a few switch/case branches in the big `free_node` function:

`src/parser/parsetree.c`

```diff
 void free_node(Node* n) {
   if (n == NULL) return;
 
   switch (n->type) {
     case T_SysCmd:
       free_syscmd((SysCmd*)n);
       break;
     case T_InsertStmt:
       free_insert_stmt((InsertStmt*)n);
       break;
+    case T_SelectStmt:
+      free_selectstmt((SelectStmt*)n);
+      break;
+    case T_ParseList:
+      free_parselist((ParseList*)n);
+      break;
+    case T_ResTarget:
+      free_restarget((ResTarget*)n);
+      break;
     default:
       printf("Unknown node type\n");
   }
 
   free(n);
 }
```

Nothing interesting to see here either.

The last thing we need to do is write `print_` functions for the new nodes:

`src/parser/parsetree.c`

```diff
+static void print_restarget(ResTarget* r) {
+  printf("%s", r->name);
+}
+
+static void print_selectstmt(SelectStmt* s) {
+  printf("=  Type: Select\n");
+  printf("=  Targets:\n");
+
+  if (s->targetList == NULL || s->targetList->length == 0) {
+    printf("=    (none)\n");
+    return;
+  }
+
+  for (int i = 0; i < s->targetList->length; i++) {
+    printf("=    ");
+    print_restarget((ResTarget*)s->targetList->elements[i].ptr);
+    printf("\n");
+  }
+}
+
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
+    case T_SelectStmt:
+      print_selectstmt((SelectStmt*)n);
+      break;
     default:
       printf("print_node() | unknown node type\n");
   }
 }
```

Again, not particularly interesting. But at least we're done writing code. Let's run the program and see how we did.

## Running the Program

```shell
$ make && ./burkeql
======   BurkeQL Config   ======
= DATA_FILE: /home/burke/source_control/burkeql-db/db_files/main.dbd
= PAGE_SIZE: 128
bql > select foo, bar, baz;
======  Node  ======
=  Type: Select
=  Targets:
=    foo
=    bar
=    baz
bql >
```

And our parser was able to parse each of our target columns as expected.