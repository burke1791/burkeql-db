
%define api.pure true

%{

#include <stdio.h>
#include <stdarg.h>
#include "include/parser/parsetree.h"

extern int yylineno;

%}

%union {
  char* str;

  struct Node* node;
}

%parse-param { struct Node** n }

%token <str> SYS_CMD

/* reserved keywords in alphabetical order */
%token INSERT

%token SELECT

%type <node> query cmd stmt sys_cmd select_stmt insert_stmt

%start query

%%

query: cmd  { 
      *n = $$ = $1;
      return 0;
    }
  ;

cmd: stmt
  | sys_cmd 
  ;

sys_cmd: SYS_CMD {
      SysCmd* n = create_node(SysCmd);

      n->cmd = $1;

      $$ = (Node*)n;
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

void yyerror(char* s, ...) {
  extern yylineno;

  va_list ap;
  va_start(ap, s);

  fprintf(stderr, "error on line %d: ", yylineno);
  vfprintf(stderr, s, ap);
  fprintf(stderr, "\n");
}