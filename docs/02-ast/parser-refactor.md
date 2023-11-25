# Parser Refactor

Now we've come to the potentially confusing part where we reconfigure flex and bison. Again, I will not be thoroughly explaining how these changes affect what flex and bison do under the hood. However, I will explain how it changes how we interact with the parser.

You may have noticed the parser we built in the first section consumed our input and printed out a message. Our call to `yyparse` did not return anything, so there was nothing for us to send along to a downstream process. We need to reconfigure the parser to be able to return an AST to the caller.

## Flex

The changes to our scanner are small, but they do introduce a new flex concept that I didn't cover in the last section: start states.

```diff
 %option noyywrap nodefault case-insensitive
+%option bison-bridge reentrant
+%option header-file="scan.lex.h"
+
+%x SYSCMD

 %{
```

Simply put, two new options `bison-bridge` and `reentrant` work with the bison changes to allow it to work with our own custom structs and return them to us. The second new option line tells flex to generate a header file for us, which will be important when we refactor the code that calls the parser.

Finally, the `%x SYSCMD` is what flex calls a start state, more specifically, an exclusive start state. This allows us to control which regex patters can be matched and when.

```diff
 %%
 
+  /* beginning a system command */
+[\\]      { BEGIN SYSCMD; }
+
+  /* valid system command inputs */
+<SYSCMD>[A-Za-z]+   { yylval->str = strdup(yytext); return SYS_CMD; }
+
   /* keywords */
 INSERT    { return INSERT; }

-QUIT      { return QUIT; }
```

The first regex pattern, `[\\]` tells flex what to look for in order to enter the `SYSCMD` state. The second new pattern is tagged with the start state and means flex is only allowed to match this pattern if it is already in the `SYSCMD` state.

Once matched, we `strdup` the text into the `yylval` union and send it along to bison. The `yylval` union matches exactly the union we'll define in our grammar file. How all of this interplay works between flex and bison is not something I'll cover, so if you're interested you should read the documentation for each tool.

And since we're changing how system commands are parsed, we no longer need the `QUIT` token.

## Bison

The changes to our grammar are a lot more involved because we're essentially rewriting how it parses tokens from scratch.

### Config

```diff
+%define api.pure true
 
 %{
 
 #include <stdio.h>
 #include <stdarg.h>
+#include "include/parser/parsetree.h"
 
 %}
```

The new `%define` turns bison into a pure parser, which for us means we're able to pass in a struct when we call it. This is absolutely necessary since we want bison to build an AST for us. The struct we send to bison is the root node of our AST.

We also need to include the AST header file so bison has access to the necessary structs and functions.

### Data Types

This next set of changes defines the data interface between flex and bison.

```diff
 %}

+%union {
+  char* str;
+
+  struct Node* node;
+}
+
+%parse-param { struct Node** n }
+%param { void* scanner }
```

As I mentioned above, the `%union` determines how the `yylval` union available to flex is defined. Anything we put in this union is settable by flex during the lexing stage.

The `%parse-param` and `%param` add arguments to the `yyparse` function. `%parse-param` allows us to send our struct to bison so that it can build the AST for us. `%param` also lets us send data, but in this case bison forwards it along to `yylex` from flex. This is necessary because we are now setting up our own scanner instead of letting flex do it for us.

Next, we need to define our new tokens and types:

```diff
+%token <str> SYS_CMD
+
 /* reserved keywords in alphabetical order */
 %token INSERT
-
-%token QUIT

 %token SELECT

+%type <node> cmd stmt sys_cmd select_stmt insert_stmt
+
-%start cmd
+%start query

 %%
```

Since we added a way for flex to tokenize a `SYS_CMD`, we need to define that token in our grammar file. And since we actually care WHAT it is that flex matched, we define a `SYS_CMD` as a `str` type. Note, type definitions like this MUST be defined in the `%union` before we can use them in the `%token` and `%type` tags.

Now that we have our AST defined in the parsetree header file, we know that every node in the tree is going to be a `Node` struct. So we tell bison that all of our rules defined below are `<node>` types.

And finally, I'm changing the `%start` from `cmd` to a new rule called `query`. This means I only need to write the termination code once.

### Grammar

And finally we get to the rules. We're rewriting everything from scratch, so you can start by deleting everything in the second section of your grammar file (between the two `%%` symbols). This chunk of code is the entirety of our grammar at this point.

```c
%%

query: cmd  { 
      *n = $1;
      YYACCEPT;
    }
  ;

cmd: stmt
  | sys_cmd 
  ;

sys_cmd: SYS_CMD {
      SysCmd* sc = create_node(SysCmd);

      sc->cmd = $1;

      $$ = (Node*)sc;
    }
  ;

stmt: select_stmt
  | insert_stmt
  ;

select_stmt: SELECT {
      printf("SELECT command received\n");
      $$ = NULL;
    }
  ;

insert_stmt: INSERT {
      printf("INSERT command received\n");
      $$ = NULL;
    }
  ;

%%
```

I've added some bison functionality that wasn't necessary in the first iteration, so it might be a little confusing right now, but stay with me.

Starting at the bottom with the `select_stmt` and `insert_stmt` rules. You'll notice they're largely the same as before, but we added a `$$ = NULL;` line. `$$` is special bison syntax. In this case `$$` represents the left side of our grammar rule definition. So in the `insert_stmt` rule, when we write `$$ = NULL;`, we're saying `insert_stmt = NULL` and in any other rules that `insert_stmt` bubbles up to its value will be `NULL`.

Moving up to the `stmt` rule, we see it's defined as either a `select_stmt` or `insert_stmt`, but there's no code attached to it. Bison actually injects default code to all rules we don't specify them for. The default action is `{ $$ = $1; }`. Just like `$$` represents the left side of a rule, `$1` corresponds to the right side of a rule - specifically, the first term of the right side of the rule. This will come in to play more when we start defining rules with multiple terms.

Note: in the `stmt` rule, `select_stmt` and `insert_stmt` are separated by a pipe (`|`), meaning they're independent. Bison injects the default code for BOTH of them. This means they're both the first term on the right side of the rule. It's the same as writing this:

```c
stmt: select_stmt { $$ = $1; }
  | insert_stmt { $$ = $1; }
  ;
```

Writing a rule like this is just a way to group similar rules together so that upstream rules only need to write a single code block instead of separate identical code blocks.

Next, we get to the `sys_cmd` rule. When we have a more robust parser, this is what a lot of our grammar will look like.

```c
sys_cmd: SYS_CMD {
      SysCmd* sc = create_node(SysCmd);

      sc->cmd = $1;

      $$ = (Node*)sc;
    }
  ;
```

We start by creating a new `Node` with the helper macro we wrote in the parsetree header. Then we set the `cmd` property to whatever flex tokenized and sent to bison - `$1`. And finally we set the value of the left-hand side of the rule to the `SysCmd` node. Since we defined the `sys_cmd` rule to be a node type, we have to cast `SysCmd*` to `Node*`.

Moving up the rules section, we see the `cmd` rule is similar to the `stmt` rule - just grouping things together.

Finally, we get to the top of the tree: the `query` rule, which has some code we haven't seen before. When we get to this point, we set `*n = $1;`. We know `$1` is equal to whatever previous processing stored in the result of the `cmd` rule, but what is `*n`?

Remember in the first section of the grammar file, we defined `%parse-param { struct Node** n }`. That's what `n` is referring to, and `*n` just means we're dereferencing it. This is important because the `%parse-param` is one of the arguments we supply to `yyparse` when we call it, and setting `*n = $1` is how bison returns the AST back to the caller.

The next line, `YYACCEPT;`, just tells bison we've hit the end of our parsing journey and are happy with the result.

## Flex to Bison Walkthrough

Let's take a step back and walk through how we get from user input to the `sys_cmd` code block.

When a user types `\quit` in the terminal, flex starts reading one character at a time. It sees the `\` character and immediately recognizes this as matching the `SYSCMD` start state pattern, i.e.

```c
  /* beginning a system command */
[\\]      { BEGIN SYSCMD; }
```

Now everything following the `\` is matched by the exclusive `SYSCMD` patterns, which in our case is any number of alphabet characters, i.e.

```c
  /* valid system command inputs */
<SYSCMD>[A-Za-z]+   { yylval->str = strdup(yytext); return SYS_CMD; }
```

Flex tries to match the longest chunk of input it can, so the entirety of the word `quit` is matched and stored in the `yytext` variable. This variable does not stick around, so we need to `strdup` it and store it in `yylval->str` and return the `SYS_CMD` token.

Bison then consumes the token and finds the applicable grammar rule for it, `sys_cmd: SYS_CMD`. Since we defined `%token <str> SYS_CMD` as corresponding to the `char* str;` property of the `%union`, that's what becomes available in the `$1` value. That's a lot to keep track of, I know, but here are the applicable pieces of the grammar file:

```c
%union {
  char* str;
  ...
}

%token <str> SYS_CMD

%%

sys_cmd: SYS_CMD {
      SysCmd* sc = create_node(SysCmd);

      sc->cmd = $1;

      $$ = (Node*)sc;
    }
  ;
```

When flex sets `yylval->str = "quit";`, bison does some magic behind the scenes and essentially sets `$1 = "quit";`. Which means we can set `sc->cmd = $1;` and that stores the `"quit"` string in the `cmd` property.

It's a lot to wrap your head around, but as we continue to add rules to our grammar it will begin to make a lot more sense.