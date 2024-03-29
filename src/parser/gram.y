
%define api.pure true

%{

#include <stdio.h>
#include <stdarg.h>
#include "include/parser/parsetree.h"

%}

%union {
  char* str;
  long long numval;

  int i;

  struct Node* node;
  struct ParseList* list;
}

%parse-param { struct Node** n }
%param { void* scanner }

%token <str> SYS_CMD STRING IDENT

%token <numval> NUMBER

/* reserved keywords in alphabetical order */
%token KW_FALSE

%token INSERT

%token SELECT

%token KW_TRUE

%type <node> cmd stmt sys_cmd select_stmt insert_stmt target

%type <list> target_list

%type <i> bool

%start query

%%

query: cmd  { 
      *n = $1;
      YYACCEPT;
    }
  ;

cmd: stmt ';'
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

select_stmt: SELECT target_list {
      SelectStmt* s = create_node(SelectStmt);
      s->targetList = $2;
      $$ = (Node*)s;
    }
  ;

target_list: target {
      $$ = create_parselist($1);
    }
  | target_list ',' target {
      $$ = parselist_append($1, $3);
    }
  ;

target: IDENT {
      ResTarget* r = create_node(ResTarget);
      r->name = $1;
      $$ = (Node*)r;
    }
  ;

insert_stmt: INSERT NUMBER STRING STRING NUMBER  {
      InsertStmt* ins = create_node(InsertStmt);
      ins->personId = $2;
      ins->firstName = str_strip_quotes($3);
      ins->lastName = str_strip_quotes($4);
      ins->age = $5;
      $$ = (Node*)ins;
    }
  ;

bool: KW_FALSE {
      $$ = 0;
    }
  | KW_TRUE {
      $$ = 1;
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
