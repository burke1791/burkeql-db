# Parser Refactor

Next up, we need to update our parser grammar to include the new columns in our insert statement. Remember, the table we're working with now is defined as:

```sql
Create Table person (
    person_id Int Not Null,
    name Char(20) Not Null,
    age TinyInt Not Null,
    daily_steps SmallInt Not Null,
    distance_from_home BigInt Not Null,
    is_alive Bool Not Null
);
```

Currently our insert statement syntax looks like this:

```shell
bql > insert [person_id] '[name]' [age] [daily_steps] [distance_from_home];
```

We're going to change it to this:

```shell
bql > insert [person_id] '[name]' [age] [daily_steps] [distance_from_home] [is_alive];
```

And fortunately, this type of change is very easy to make.

## Lexer Update

This is something we did not have to do last time. We need to update our lexer to match the strings `True` and `False`. That way our grammar knows to set the `isAlive` property of the insert node to either 1 or 0. Even though we use a `uint8_t` C type, we don't want to allow its value to be anything other than 1 or 0.

`src/parser/scan.l`

```diff
   /* keywords */
+FALSE     { return KW_FALSE; }
 
 INSERT    { return INSERT; }
 
 SELECT    { return SELECT; }
 
+TRUE      { return KW_TRUE; }
```

We match the literal strings "FALSE" and "TRUE" and return their associated tokens to bison.

## Grammar Update

`src/parser/gram.y`

```diff
 %union {
   char* str;
   long long numval;
 
+  int i;
 
   struct Node* node;
   struct ParseList* list;
 }
```

I'm adding an `int` to the union because I don't want to use a `long long` to represent random numbers here and there. In this case, an `int` will convert to a `uint8_t` just fine because we're constraining the values to 1 and 0.

```diff
 /* reserved keywords in alphabetical order */
+%token KW_FALSE
 
 %token INSERT
 
 %token SELECT
 
+%token KW_TRUE
 
 %type <node> cmd stmt sys_cmd select_stmt insert_stmt target
 
 %type <list> target_list
 
+%type <i> bool
 
 %start query
```

We add our new tokens and a new type for the new grammar rule we're introducing below.

```diff
-insert_stmt: INSERT NUMBER STRING NUMBER NUMBER NUMBER  {
+insert_stmt: INSERT NUMBER STRING NUMBER NUMBER NUMBER bool  {
       InsertStmt* ins = create_node(InsertStmt);
       ins->personId = $2;
       ins->name = str_strip_quotes($3);
       ins->age = $4;
       ins->dailySteps = $5;
       ins->distanceFromHome = $6;
+      ins->isAlive = $7;
       $$ = (Node*)ins;
     }
   ;
 
+bool: KW_FALSE {
+      $$ = 0;
+    }
+  | KW_TRUE {
+      $$ = 1;
+    }
+  ;
```

For the `insert_stmt` rule, we simply need to add the bool to the list the same way we added the ints last time. However, we define a new `bool` grammar rule that accepts either of the new tokens and sets their value to 1 or 0 accordingly.

## Parsetree

Next, we need to update our parsetree code.

`src/include/parser/parsetree.h`

```diff
 typedef struct InsertStmt {
   NodeTag type;
   int32_t personId;
   char* name;
   uint8_t age;
   int16_t dailySteps;
   int64_t distanceFromHome;
+  uint8_t isAlive;
 } InsertStmt;
```

`src/parser/parsetree.c`

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
     case T_InsertStmt:
       printf("=  Type: Insert\n");
       printf("=  person_id:           %d\n", ((InsertStmt*)n)->personId);
       printf("=  name:                %s\n", ((InsertStmt*)n)->name);
       printf("=  age:                 %u\n", ((InsertStmt*)n)->age);
       printf("=  daily_steps:         %d\n", ((InsertStmt*)n)->dailySteps);
       printf("=  distance_from_home:  %ld\n", ((InsertStmt*)n)->distanceFromHome);
+      printf("=  is_alive:            %u\n", ((InsertStmt*)n)->isAlive);
       break;
     case T_SelectStmt:
       print_selectstmt((SelectStmt*)n);
       break;
     default:
       printf("print_node() | unknown node type\n");
   }
 }
```

Updating the node printer to show the parsed `is_alive` value. I'm printing it as `%u` because I don't want to write the extra code to translate it to TRUE or FALSE.

That ends the changes required in the parser. In the next section, we'll make updates to the actual insert functionality and finally be able to run some insert statements and test out our changes.