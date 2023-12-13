%option noyywrap nodefault case-insensitive
%option bison-bridge reentrant
%option header-file="scan.lex.h"

%x SYSCMD

%{

#include "include/parser/parsetree.h"
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

  /* numbers */
-?[0-9]+    { yylval->intval = atoi(yytext); return INTNUM; }

  /* strings */
'(\\.|''|[^'\n])*'  { yylval->str = strdup(yytext); return STRING; }
'(\\.|[^'\n])*$     { yyerror("Unterminated string %s", yytext); }

  /* everything else */
[ \t\n]   /* whitespace */
.     { yyerror("unknown character '%c'", *yytext); }

%%