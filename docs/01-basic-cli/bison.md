# Bison Grammar

Similar to flex, bison files are also divided into three sections with a `%%` symbol.

```c
/* bison config options */

%{
/* code block for #includes or external interfaces */
}%

/* grammar types, tokens, etc. */

%% /* first divider */

/* grammar definitions */

%% /* second divider */

/* c code */
```

Also similar to flex, the third section is optional. However, this time we will actually put something there.

Another note, comments in a bison file DO NOT need to be indented like they do in flex.

## Config and Types

```c
%{
#include <stdio.h>
#include <stdarg.h>

extern int yylineno;
%}

%token INSERT

%token QUIT

%token SELECT

%start cmd
```

At the top we're including some standard libraries we make use of, as well as an interface to the lexer's line number tracker.

In the next section, we define our tokens with the `%token` symbol. When bison builds this file, it takes all `%tokens` and stuffs them in an enum called `yytokentype`, which lives in bison's header file - the same file we `#include`d in our scanner.

You can define multiple tokens on a single line by separating them with a space, e.g. `%token INSERT QUIT SELECT`. However, our grammar will eventually have a lot of tokens and I've found separating them alphabetically keeps things much more organized.

The `%start` symbol is not entirely necessary for our simple grammar, but I like to include it to be explicit about the root of our grammar definition.

## Grammar

We define a grammar by defining a number of rules. These rules can be arbitrarily complex, but in our case we just have three simple tokens to handle.

```c
cmd: stmt   { return 0; }
  ;

stmt:   select_stmt
  | insert_stmt
  | quit_stmt
  ;

/* test comment */
select_stmt: SELECT   { printf("SELECT command received\n"); }
  ;

insert_stmt: INSERT   { printf("INSERT command received\n"); }
  ;

quit_stmt: QUIT     { printf("QUIT command received\n"); }
  ;
```

Starting at the top, we have a rule named `cmd`. A `cmd` consists of a `stmt` rule, and when bison is able to satisfy the `cmd` rule, it returns 0 indicating success.

Drilling deeper, a `stmt` rule can be one of three things: `select_stmt`, `insert_stmt`, or `quit_stmt`. Each of these has their own definition and associated code to run when encountered.

## Code Section

Unlike the scanner, we are actually going to write some code for the third section. We are simply implementing our own error function that prints out the line number it found the error, as well as a provided message.

```c
void yyerror(char* s, ...) {
  extern yylineno;

  va_list ap;
  va_start(ap, s);

  fprintf(stderr, "error on line %d: ", yylineno);
  vfprintf(stderr, s, ap);
  fprintf(stderr, "\n");
}
```

## Full File

`src/parser/gram.y`

```c
%{

#include <stdio.h>
#include <stdarg.h>

extern int yylineno;

%}

%token INSERT

%token QUIT

%token SELECT

%start cmd

%%

cmd: stmt   { return 0; }
  ;

stmt:   select_stmt
  | insert_stmt
  | quit_stmt
  ;

select_stmt: SELECT   { printf("SELECT command received\n"); }
  ;

insert_stmt: INSERT   { printf("INSERT command received\n"); }
  ;

quit_stmt: QUIT     { printf("QUIT command received\n"); }
  ;

%%

void yyerror(char* s, ...) {
  extern yylineno;

  va_list ap;
  va_start(ap, s);

  fprintf(stderr, "error on line %d: ", yylineno);
  vfprintf(stderr, s, ap);
  fprintf(stderr, "\n");
}
```