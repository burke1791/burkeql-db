# Parser Refactor

Next up, we need to update our parser grammar to include the new columns in our insert statement. Remember, the table we're working with now is defined as:

```sql
Create Table person (
    person_id Int Not Null,
    name Char(20) Not Null,
    age TinyInt Not Null,
    daily_steps SmallInt Not Null,
    distance_from_home BigInt Not Null
);
```

Currently our insert statement syntax looks like this:

```shell
bql > insert [person_id] '[name]';
```

We're going to change it to this:

```shell
bql > insert [person_id] '[name]' [age] [daily_steps] [distance_from_home];
```

And fortunately, this type of change is very easy to make.

## Grammar Update

Our lexer already knows how to find numbers when it scans our input (except for a small edge case I'll go over later), so we only need to change the grammar file.

`src/parser/gram.y`

```diff
-insert_stmt: INSERT INTNUM STRING  {
+insert_stmt: INSERT INTNUM STRING INTNUM INTNUM INTNUM  {
       InsertStmt* ins = create_node(InsertStmt);
       ins->personId = $2;
       ins->name = str_strip_quotes($3);
+      ins->age = $4;
+      ins->dailySteps = $5;
+      ins->distanceFromHome = $6;
       $$ = (Node*)ins;
     }
   ;
```

Yup, that's it. Just need to add a few extra `INTNUM`'s for the additional columns and store them in the `InsertStmt` node. Speaking of, we need to add those new properties to the insert struct.

`src/include/parser/parsetree.h`

```diff
+#include <stdint.h>

*** code excluded for brevity ***

 typedef struct InsertStmt {
   NodeTag type;
-  int personId;
+  int32_t personId;
   char* name;
+  uint8_t age;
+  int16_t dailySteps;
+  int64_t distanceFromHome;
 } InsertStmt;
```

While we're at it, I'm going to change `personId`'s type to more accurately reflect the data type we're storing on the page. And because these use the specialty `int` types, we need to `#include <stdint.h>`.

Note: because our additional data types are all "ints", we do not need to add any logic for freeing their memory. We do, however, have one more small update to make in the `parsetree` file:

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
+      printf("=  age:                 %u\n", ((InsertStmt*)n)->age);
+      printf("=  daily_steps:         %d\n", ((InsertStmt*)n)->dailySteps);
+      printf("=  distance_from_home:  %ld\n", ((InsertStmt*)n)->distanceFromHome);
       break;
     case T_SelectStmt:
       print_selectstmt((SelectStmt*)n);
       break;
     default:
       printf("print_node() | unknown node type\n");
   }
 }
```

I just want our info prints to show what the parser parsed out for us.

That ends the changes required in the parser. In the next section, we'll make updates to the actual insert functionality and finally be able to run some insert statements and test out our changes.