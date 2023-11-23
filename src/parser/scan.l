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