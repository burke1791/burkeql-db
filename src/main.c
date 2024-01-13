#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <strings.h>

#include "gram.tab.h"
#include "parser/parsetree.h"
#include "parser/parse.h"
#include "global/config.h"
#include "storage/file.h"
#include "storage/page.h"
#include "storage/table.h"
#include "buffer/bufpool.h"
#include "storage/table.h"
#include "resultset/recordset.h"
#include "resultset/resultset_print.h"
#include "access/tableam.h"
#include "utility/linkedlist.h"

Config* conf;

/* TEMPORARY CODE SECTION */

#define RECORD_LEN  48
#define BUFPOOL_SLOTS  1

static void populate_datum_array(Datum* fixed, Datum* varlen, int32_t person_id, char* firstName, char* lastName, int32_t age) {
  fixed[0] = int32GetDatum(person_id);
  fixed[1] = int32GetDatum(age);

  varlen[0] = charGetDatum(firstName);
  varlen[1] = charGetDatum(lastName);
}

static RecordDescriptor* construct_record_descriptor() {
  RecordDescriptor* rd = malloc(sizeof(RecordDescriptor) + (4 * sizeof(Column)));
  rd->ncols = 4;
  rd->nfixed = 2;

  construct_column_desc(&rd->cols[0], "person_id", DT_INT, 0, 4);
  construct_column_desc(&rd->cols[1], "first_name", DT_VARCHAR, 1, 20);
  construct_column_desc(&rd->cols[2], "last_name", DT_VARCHAR, 2, 20);
  construct_column_desc(&rd->cols[3], "age", DT_INT, 3, 4);

  return rd;
}

static RecordDescriptor* construct_record_descriptor_from_target_list(ParseList* targetList) {
  RecordDescriptor* rd = malloc(sizeof(RecordDescriptor) + (targetList->length * sizeof(Column)));
  rd->ncols = targetList->length;

  for (int i = 0; i < rd->ncols; i++) {
    ResTarget* t = (ResTarget*)targetList->elements[i].ptr;

    // we don't care about the data type or length here
    construct_column_desc(&rd->cols[i], t->name, DT_UNKNOWN, i, 0);
  }

  return rd;
}

void free_record_desc(RecordDescriptor* rd) {
  for (int i = 0; i < rd->ncols; i++) {
    if (rd->cols[i].colname != NULL) {
      free(rd->cols[i].colname);
    }
  }
  free(rd);
}

static void serialize_data(RecordDescriptor* rd, Record r, int32_t person_id, char* firstName, char* lastName, int32_t age) {
  Datum* fixed = malloc(rd->nfixed * sizeof(Datum));
  Datum* varlen = malloc((rd->ncols - rd->nfixed) * sizeof(Datum));

  populate_datum_array(fixed, varlen, person_id, firstName, lastName, age);
  fill_record(rd, r + sizeof(RecordHeader), fixed, varlen);
  free(fixed);
  free(varlen);
}

static int compute_record_length(RecordDescriptor* rd, int32_t person_id, char* firstName, char* lastName, int32_t age) {
  int len = 12; // start with the 12-byte header
  len += 4; // person_id

  /* for the varlen columns, we default to their max length if the values
     we're trying to insert would overflow them (they'll get truncated later) */
  if (strlen(firstName) > rd->cols[1].len) {
    len += (rd->cols[1].len + 2);
  } else {
    len += (strlen(firstName) + 2);
  }

  if (strlen(lastName) > rd->cols[2].len) {
    len += (rd->cols[2].len + 2);
  } else {
    len += (strlen(lastName) + 2);
  }

  len += 4; // age

  return len;
}

static bool insert_record(BufPool* bp, int32_t person_id, char* firstName, char* lastName, int32_t age) {
  BufPoolSlot* slot = bufpool_read_page(bp, 1);
  if (slot == NULL) slot = bufpool_new_page(bp);
  RecordDescriptor* rd = construct_record_descriptor();

  int recordLength = compute_record_length(rd, person_id, firstName, lastName, age);
  Record r = record_init(recordLength);

  serialize_data(rd, r, person_id, firstName, lastName, age);
  bool insertSuccessful = page_insert(slot->pg, r, recordLength);

  free_record_desc(rd);
  free(r);
  
  return insertSuccessful;
}

static bool analyze_selectstmt(SelectStmt* s) {
  for (int i = 0; i < s->targetList->length; i++) {
    ResTarget* r = (ResTarget*)s->targetList->elements[i].ptr;
    if (
      !(
        strcasecmp(r->name, "person_id") == 0 ||
        strcasecmp(r->name, "first_name") == 0 ||
        strcasecmp(r->name, "last_name") == 0 ||
        strcasecmp(r->name, "age") == 0
      )
    ) {
      return false;
    }
  }
  return true;
}

static bool analyze_node(Node* n) {
  switch (n->type) {
    case T_SelectStmt:
      return analyze_selectstmt((SelectStmt*)n);
    default:
      printf("analyze_node() | unhandled node type");
  }

  return false;
}

/* END TEMPORARY CODE */

static void print_prompt() {
  printf("bql > ");
}

int main(int argc, char** argv) {
  // initialize global config
  conf = new_config();

  if (!set_global_config(conf)) {
    return EXIT_FAILURE;
  }

  // print config
  print_config(conf);

  FileDesc* fdesc = file_open(conf->dataFile);
  BufPool* bp = bufpool_init(fdesc, BUFPOOL_SLOTS);

  while(true) {
    print_prompt();
    Node* n = parse_sql();

    if (n == NULL) continue;

    print_node(n);

    switch (n->type) {
      case T_SysCmd:
        if (strcmp(((SysCmd*)n)->cmd, "quit") == 0) {
          free_node(n);
          printf("Shutting down...\n");
          bufpool_flush_all(bp);
          bufpool_destroy(bp);
          file_close(fdesc);
          return EXIT_SUCCESS;
        }
        break;
      case T_InsertStmt: {
        int32_t person_id = ((InsertStmt*)n)->personId;
        char* firstName = ((InsertStmt*)n)->firstName;
        char* lastName = ((InsertStmt*)n)->lastName;
        int32_t age = ((InsertStmt*)n)->age;
        if (!insert_record(bp, person_id, firstName, lastName, age)) {
          printf("Unable to insert record\n");
        }
        break;
      }
      case T_SelectStmt:
        if (!analyze_node(n)) {
          printf("Semantic analysis failed\n");
        } else {
          TableDesc* td = new_tabledesc("person");
          td->rd = construct_record_descriptor();
          RecordSet* rs = new_recordset();
          rs->rows = new_linkedlist();
          RecordDescriptor* targets = construct_record_descriptor_from_target_list(((SelectStmt*)n)->targetList);
          
          tableam_fullscan(bp, td, rs->rows);
          resultset_print(td->rd, rs, targets);

          free_recordset(rs, td->rd);
          free_tabledesc(td);
          free_record_desc(targets);
        }
        break;
    }

    free_node(n);
  }

  return EXIT_SUCCESS;
}