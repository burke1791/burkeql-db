%option noyywrap nodefault case-insensitive
%option bison-bridge reentrant
%option header-file="scan.lex.h"

%x SYSCMD

%{

#include "gram.tab.h"

void yyerror(char* s, ...);

%}

%%

  /* beginning a system command */
[\\]      { BEGIN SYSCMD; }

  /* valid system command inputs */
<SYSCMD>[A-Za-z]+   { yylval->str = strdup(yytext); return SYS_CMD; }

  /* keywords */
INSERT    { return INSERT; }

SELECT    { return SELECT; }

  /* everything else */
[ \t\n]   /* whitespace */
.     { yyerror("unknown character '%c'", *yytext); }

%%