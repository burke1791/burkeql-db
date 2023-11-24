
%{

#include <stdio.h>
#include <stdarg.h>

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
  va_list ap;
  va_start(ap, s);

  fprintf(stderr, "error: ");
  vfprintf(stderr, s, ap);
  fprintf(stderr, "\n");
}