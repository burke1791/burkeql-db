
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

%token KW_NULL

%token SELECT

%token KW_TRUE

%type <node> cmd stmt sys_cmd select_stmt insert_stmt target literal

%type <list> target_list literal_values_list

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

insert_stmt: INSERT literal_values_list  {
      InsertStmt* ins = create_node(InsertStmt);
      ins->values = $2;
      
      $$ = (Node*)ins;
    }
  ;

literal_values_list: literal {
      $$ = create_parselist($1);
    }
  | literal_values_list literal {
      $$ = parselist_append($1, $2);
    }
  ;

literal: NUMBER {
      Literal* l = create_node(Literal);
      l->str = NULL;
      l->intVal = $1;
      l->isNull = false;
      
      $$ = (Node*)l;
    }
  | STRING {
      Literal* l = create_node(Literal);
      l->str = str_strip_quotes($1);
      l->isNull = false;

      $$ = (Node*)l;
    }
  | KW_NULL {
      Literal* l = create_node(Literal);
      l->str = NULL;
      l->isNull = true;

      $$ = (Node*)l;
    }
  | KW_FALSE {
      Literal* l = create_node(Literal);
      l->str = NULL;
      l->boolVal = false;
      l->isNull = false;

      $$ = (Node*)l;
    }
  | KW_TRUE {
      Literal* l = create_node(Literal);
      l->str = NULL;
      l->boolVal = true;
      l->isNull = false;

      $$ = (Node*)l;
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
