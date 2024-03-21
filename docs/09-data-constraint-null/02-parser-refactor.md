# Parser Refactor

This is the last time we'll have to refactor code that we know is going to be thrown out in the future. Hurray! But I must begin with an apology. The amount of refactoring we need to do is quite significant, and it's made worse knowing we're going to toss it all out when we implement system catalogs soon.

But this is the last time we'll have to write a bunch of throwaway code, so that's something!

You're probably wondering why the refactor will be so large since our table is pretty much the same. Well, in order to support `Null`'s, we need to restructure our parser's insert statement grammar, which means we need to redo the `InsertStmt` struct, which means all of our temporary code that works with that struct needs to change as well. It's a real house of cards/trail of dominos situation.

Let's just dive right in, starting with the parser.

## Scanner

As a reminder, our table DDL looks like this:

```sql
Create Table person (
    person_id Int Not Null,
    first_name Varchar(20) Null,
    last_name Varchar(20) Not Null,
    age Int Null
);
```

We have two columns that can store `Null` values. And in order to consume `Null`'s from the client, we need to update our scanner and grammar to be able to parse them.

From the client's perspective, we want to consume `Null`'s like this:

```shell
bql > insert 1 Null 'Burke' Null;
```

The `insert` command still expects four inputs, but in order to indicate you want a `Null` in a given field, you just pass in the literal keyword: "Null".

Fortunately, the scanner update is quite simple:

`src/parser/scan.l`

```diff
 INSERT    { return INSERT; }
 
+NULL      { return KW_NULL; }
 
 SELECT    { return SELECT; }
```

We just add a new "NULL" keyword.

## Grammar

The grammar, on the other hand, is a lot more complex by comparison.

`src/parser/gram.y`

```diff
 %token INSERT
 
+%token KW_NULL
 
 %token SELECT
 
 %token KW_TRUE
 
-%type <node> cmd stmt sys_cmd select_stmt insert_stmt target
+%type <node> cmd stmt sys_cmd select_stmt insert_stmt target literal

-%type <list> target_list
+%type <list> target_list literal_values_list
 
 %start query
```

We have to add a new token for `KW_NULL` to match what the new scanner token. Then we add two new rules, a `<node>` type called `literal`, and a `<list>` type called `literal_values_list`. As you can probably guess, the latter is just a `ParseList` of the former. More on these below.

`src/parser/gram.y`

```diff
-insert_stmt: INSERT NUMBER STRING STRING NUMBER {
+insert_stmt: INSERT literal_values_list  {
       InsertStmt* ins = create_node(InsertStmt);
-      ins->personId = $2;
-      ins->firstName = str_strip_quotes($3);
-      ins->lastName = str_strip_quotes($4);
-      ins->age = $5;
+      ins->values = $2;
       
       $$ = (Node*)ins;
     }
   ;

+literal_values_list: literal {
+      $$ = create_parselist($1);
+    }
+  | literal_values_list literal {
+      $$ = parselist_append($1, $2);
+    }
+  ;
```

The big change in the insert grammar is instead of explicitly defined input rules for each of the four inputs, we now consume a list. And that list pattern is almost exactly the same pattern we use for the `target_list` in the select statement grammar.

Notice that instead of individual fields hard-coded in the `InsertStmt` node, we have a new field called `values`. This is the big change that necessitates a bunch of downstream refactoring in our temporary code. But more on that later.

What the heck is a `literal`?

`src/parser/gram.y`

```diff
-bool: KW_FALSE {
-      $$ = 0;
-    }
-  | KW_TRUE {
-      $$ = 1;
-    }
-  ;

+literal: NUMBER {
+      Literal* l = create_node(Literal);
+      l->str = NULL;
+      l->intVal = $1;
+      l->isNull = false;
+      
+      $$ = (Node*)l;
+    }
+  | STRING {
+      Literal* l = create_node(Literal);
+      l->str = str_strip_quotes($1);
+      l->isNull = false;
+
+      $$ = (Node*)l;
+    }
+  | KW_NULL {
+      Literal* l = create_node(Literal);
+      l->str = NULL;
+      l->isNull = true;
+
+      $$ = (Node*)l;
+    }
+  | KW_FALSE {
+      Literal* l = create_node(Literal);
+      l->str = NULL;
+      l->boolVal = false;
+      l->isNull = false;
+
+      $$ = (Node*)l;
+    }
+  | KW_TRUE {
+      Literal* l = create_node(Literal);
+      l->str = NULL;
+      l->boolVal = true;
+      l->isNull = false;
+
+      $$ = (Node*)l;
+    }
+  ;
```

First, we toss out the `bool` grammar rule because we don't need it anymore. Then we define our `literal`. Glancing at the five different ways a literal is defined, you can see they just correspond to the different data types.

Starting from the top, our first literal corresponds to the `NUMBER` token and is just an `int`. You'll notice we're creating a new node type, called `Literal`, which we'll need to define. But suffice to say, the `Literal` struct has a field for each type of data we support. `NUMBER`'s get assigned to the `intVal` field, `STRING`'s are assigned to the `str` field, and so on.

Two noteworthy things to mention. (1) the `isNull` field is set in all cases. And (2) we set `str` to `NULL` for all non-`STRING` data types.

The first, `isNull`, is super important for downstream processing. Our `fill_val` function needs to know if the incoming data is null or not so it can either write data to the record or not.

The second, `str`, is important simply because it is a pointer. All unused pointers must be explicitly set to `NULL` otherwise any access to them, even to attempt freeing the memory, is undefined behavior. So we MUST set it to `NULL` if we're not using it.

## Parsetree Refactor

Now that our scanner and grammar refactor is complete, let's update our parsetree structs and code.

`src/include/parser/parsetree.h`

```diff
 #ifndef PARSETREE_H
 #define PARSETREE_H
 
 #include <stdint.h>
+#include <stdbool.h>

 typedef enum NodeTag {
   T_SysCmd,
   T_InsertStmt,
   T_SelectStmt,
   T_ParseList,
   T_ResTarget,
+  T_Literal
 } NodeTag;
```

We need to include a new standard library to support using the `bool` primitive in our new `Literal` struct below. And we also add the `T_Literal` node tag to the enum.

`src/include/parser/parsetree.h`

```diff
 typedef struct InsertStmt {
   NodeTag type;
-  int32_t personId;
-  char* firstName;
-  char* lastName;
-  int32_t age;
+  ParseList* values;
 } InsertStmt;
 
 typedef struct ResTarget {
   NodeTag type;
   char* name;
 } ResTarget;
 
 typedef struct SelectStmt {
   NodeTag type;
   ParseList* targetList;
 } SelectStmt;
 
+typedef struct Literal {
+  NodeTag type;
+  bool isNull;
+  int64_t intVal;
+  char* str;
+  bool boolVal;
+} Literal;
```

Next we delete the hard-coded fields in the `InsertStmt` struct and replace them with a more flexible `ParseList*` field. This is how the grammar can stash its `literal_values_list` grammar rule into the `InsertStmt` node.

Next, we define the `Literal` struct. It has a field for each of the data type we support, `bool`, `int`'s, and `char*`, as well as a meta `isNull` field. The `int` is an 8-byte integer because it's the largest number type our database supports and upsize conversions are lossless. If the client tries to input a number that would overflow the data type.. well that's a problem for the analyzer - which we will not implement yet.

Anyways, moving on to the parsetree code.

`src/parser/parsetree.c`

```diff
 static void free_insert_stmt(InsertStmt* ins) {
   if (ins == NULL) return;
 
-  free(ins->firstName);
-  free(ins->lastName);
+  free_parselist(ins->values);
+  free(ins->values);
 }

+static void free_literal(Literal* l) {
+  if (l->str != NULL) {
+    free(l->str);
+  }
+}

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
+    case T_Literal:
+      free_literal((Literal*)n);
+      break;
     default:
       printf("Unknown node type\n");
   }
 
   free(n);
 }
```

First, we update code for freeing the memory consumed by our refactored `InsertStmt` node and the new `Literal` node. Pretty straightforward.

`src/parser/parsetree.c`

```diff
 #include "parser/parsetree.h"
+#include "storage/record.h"

*** omitted for brevity ***

+// Probably a temporary function
+static void print_insertstmt_literal(Literal* l, char* colname, DataType dt) {
+  int padLen = 20 - strlen(colname);
+
+  if (l->isNull) {
+    printf("=  %s%*sNULL\n", colname, padLen, " ");
+  } else {
+    switch (dt) {
+      case DT_VARCHAR:
+        printf("=  %s%*s%s\n", colname, padLen, " ", l->str);
+        break;
+      case DT_INT:
+        printf("=  %s%*s%d\n", colname, padLen, " ", (int32_t)l->intVal);
+        break;
+    }
+  }
+}
 
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
     case T_InsertStmt: {
+      InsertStmt* i = (InsertStmt*)n;
       printf("=  Type: Insert\n");
-      printf("=  person_id:           %d\n", ((InsertStmt*)n)->personId);
-      printf("=  first_name:          %s\n", ((InsertStmt*)n)->firstName);
-      printf("=  last_name:           %s\n", ((InsertStmt*)n)->lastName);
-      printf("=  age:                 %d\n", ((InsertStmt*)n)->age);
+      print_insertstmt_literal((Literal*)i->values->elements[0].ptr, "person_id", DT_INT);
+      print_insertstmt_literal((Literal*)i->values->elements[1].ptr, "first_name", DT_VARCHAR);
+      print_insertstmt_literal((Literal*)i->values->elements[2].ptr, "last_name", DT_VARCHAR);
+      print_insertstmt_literal((Literal*)i->values->elements[3].ptr, "age", DT_INT);
       break;
     }
     case T_SelectStmt:
       print_selectstmt((SelectStmt*)n);
       break;
     default:
       printf("print_node() | unknown node type\n");
   }
 }
```

Next, we restructure how our `print_node` function works. Instead of explicitly printing the hard-coded fields of the `InsertStmt` node, we create a helper function (probably temporary, ugh!), that prints what we want.

The reason we're using a temporary function instead of drilling into the `Literal` struct in a `printf` call is because we need to check if the value is `Null` first. If the value is `Null`, then we just want to print the string "NULL", otherwise we print its value.

Since our table only has `Varchar` and `Int` types, I'm only including `DT_VARCHAR` and `DT_INT` in the switch/case block of the helper function. If we had more than just those columns, we would need to voer those types.

That concludes our parser refactor. In the next section we're going to refactor how our storage engine writes data to a record. And it gets interesting because we're going to delve into the wonderful world of bitfield manipulation.