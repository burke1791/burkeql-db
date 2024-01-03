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
FALSE     { return KW_FALSE; }

INSERT    { return INSERT; }

SELECT    { return SELECT; }

TRUE      { return KW_TRUE; }

  /* numbers */
-?[0-9]+    { yylval->numval = strtoll(yytext, &yytext, 10); return NUMBER; }

  /* operators */
[,;]   { return yytext[0]; }

  /* strings */
'(\\.|''|[^'\n])*'  { yylval->str = strdup(yytext); return STRING; }

  /* identifiers */
[A-Za-z_][A-Za-z0-9_]*   { yylval->str = strdup(yytext); return IDENT; }

  /* everything else */
[ \t\n]   /* whitespace */
.     { yyerror("unknown character '%c'", *yytext); }

%%