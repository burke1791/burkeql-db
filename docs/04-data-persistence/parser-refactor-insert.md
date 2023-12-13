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

