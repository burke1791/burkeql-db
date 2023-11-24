# A Flex Scanner

Flex files have three sections separated by a `%%` symbol:

```c
  /* flex config options, e.g. */
%option case-insensitive

%{

/* code block for #includes or providing an interface to external functions */

}%

%% /* 1st divider */

  /* This second section is where we define regex patters for matching tokens from the input, e.g. */

  /* matches any alphanumeric pattern and returns the matched token */
[a-zA-Z0-9]+    { return yytext; }

%% /* 2nd divider */

/* The third section is where we can define C functions for use within the lexer */
```

The third section is entirely optional. If you want to use functions that are external to the flex file, you simply need to `#include` the appropriate header in the first section's code block.

Enough with the intro, let's get started.

## Config

```c
%option noyywrap nodefault case-insensitive

%{
  #include "gram.tab.h"

  void yyerror(char* s, ...);
}%
```

Again, I'm not deep-diving flex here so I won't be going into the `%options`, but just know we need all three of them for the scanner to behave how we want.

In the code block we include the header file that bison produces (explained in the next section), as well as an interface to bison's error function.

## Regex

The second section is pretty straightforward. We're simply defining regex patters for our scanner to match against its input.

Currently, we're implementing only three commands: `insert`, `quit`, and `select`. Therefore, we need a match pattern for each one.

```c
  /* keywords (alphabetical order) */
INSERT      { return INSERT; }

QUIT        { return QUIT; }

SELECT      { return SELECT; }

  /* everything else */
[ \t\n]     /* whitespace - ignore */
.           { yyerror("unknown character '%c'", *yytext); }
```

A couple things are going on here. At the beginning of the line, we specify our regex pattern. Since we want to match our three commands exactly, the pattern is just the command itself. After the regex pattern, we see code in curly brackets - this is what flex runs when it finds a match. If the word "insert" comes along, flex will match it and `return` the `INSERT` token to bison. 

How does flex know that `INSERT` is a valid token? Bison defines all tokens in its `gram.tab.h` header file that we included at the top.

**Note: comments in this section MUST be indented.** Flex assumes anything at the start of the line is a regex pattern and will throw a compilation error.

## Code Section

Empty.

## Full File

`src/parser/scan.l`

```c
%option noyywrap nodefault yylineno case-insensitive

%{

#include "gram.tab.h"

void yyerror(char* s, ...);

%}

%%

  /* keywords */
INSERT    { return INSERT; }

QUIT      { return QUIT; }

SELECT    { return SELECT; }

  /* everything else */
[ \t\n]   /* whitespace */
.     { yyerror("unknown character '%c'", *yytext); }

%%
```