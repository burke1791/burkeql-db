
%define api.pure true

%{

#include <stdio.h>
#include <stdarg.h>
#include "include/parser/parsetree.h"

%}

%union {
  char* str;

  struct Node* node;
}

%parse-param { struct Node** n }
%param { void* scanner }

%token <str> SYS_CMD

/* reserved keywords in alphabetical order */
%token INSERT

%token SELECT

%type <node> cmd stmt sys_cmd select_stmt insert_stmt

%start query

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

void yyerror(char* s, ...) {
  va_list ap;
  va_start(ap, s);

  fprintf(stderr, "error: ");
  vfprintf(stderr, s, ap);
  fprintf(stderr, "\n");
}