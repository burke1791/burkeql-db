#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <strings.h>

#include "gram.tab.h"
#include "parser/parsetree.h"
#include "parser/parse.h"
#include "parser/analyze.h"
#include "global/config.h"
#include "storage/page.h"
#include "storage/table.h"
#include "buffer/bufmgr.h"
#include "storage/table.h"
#include "resultset/recordset.h"
#include "resultset/resultset_print.h"
#include "access/tableam.h"
#include "utility/linkedlist.h"
#include "system/syscmd.h"
#include "system/initdb.h"

Config* conf;

/* TEMPORARY CODE SECTION */

#define RECORD_LEN  48
#define BUFPOOL_SLOTS  1

static void populate_datum_array(Datum* fixed, Datum* varlen, bool* fixedNull, bool* varlenNull, ParseList* values) {
  Literal* personId = (Literal*)values->elements[0].ptr;
  Literal* firstName = (Literal*)values->elements[1].ptr;
  Literal* lastName = (Literal*)values->elements[2].ptr;
  Literal* age = (Literal*)values->elements[3].ptr;

  fixed[0] = int32GetDatum(personId->intVal);
  fixedNull[0] = false;

  if (age->isNull) {
    fixed[1] = (Datum)NULL;
    fixedNull[1] = true;
  } else {
    fixed[1] = int32GetDatum(age->intVal);
    fixedNull[1] = false;
  }
  
  if (firstName->isNull) {
    varlen[0] = (Datum)NULL;
    varlenNull[0] = true;
  } else {
    varlen[0] = charGetDatum(firstName->str);
    varlenNull[0] = false;
  }
  
  varlen[1] = charGetDatum(lastName->str);
  varlenNull[1] = false;
}

static RecordDescriptor* construct_record_descriptor() {
  RecordDescriptor* rd = malloc(sizeof(RecordDescriptor) + (4 * sizeof(Column)));
  rd->ncols = 4;
  rd->nfixed = 2;

  construct_column_desc(&rd->cols[0], "person_id", DT_INT, 0, 4, true);
  construct_column_desc(&rd->cols[1], "first_name", DT_VARCHAR, 1, 20, false);
  construct_column_desc(&rd->cols[2], "last_name", DT_VARCHAR, 2, 20, true);
  construct_column_desc(&rd->cols[3], "age", DT_INT, 3, 4, false);

  rd->hasNullableColumns = true;

  return rd;
}

static RecordDescriptor* construct_record_descriptor_from_target_list(ParseList* targetList) {
  RecordDescriptor* rd = malloc(sizeof(RecordDescriptor) + (targetList->length * sizeof(Column)));
  rd->ncols = targetList->length;

  for (int i = 0; i < rd->ncols; i++) {
    ResTarget* t = (ResTarget*)targetList->elements[i].ptr;

    // we don't care about the data type, length, or nullability here
    construct_column_desc(&rd->cols[i], t->name, DT_UNKNOWN, i, 0, true);
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

static void serialize_data(RecordDescriptor* rd, Record r, ParseList* values) {
  Datum* fixed = malloc(rd->nfixed * sizeof(Datum));
  Datum* varlen = malloc((rd->ncols - rd->nfixed) * sizeof(Datum));
  bool* fixedNull = malloc(rd->nfixed * sizeof(bool));
  bool* varlenNull = malloc((rd->ncols - rd->nfixed) * sizeof(bool));

  populate_datum_array(fixed, varlen, fixedNull, varlenNull, values);

  int nullOffset = sizeof(RecordHeader) + compute_record_fixed_length(rd, fixedNull);
  ((RecordHeader*)r)->nullOffset = nullOffset;

  uint8_t* nullBitmap = r + nullOffset;
  fill_record(rd, r + sizeof(RecordHeader), fixed, varlen, fixedNull, varlenNull, nullBitmap);

  free(fixed);
  free(varlen);
  free(fixedNull);
  free(varlenNull);
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

  BufMgr* buf = bufmgr_init();

  if (!initdb(buf)) {
    printf("initdb failed\n");
    printf("Shutting down...\n");
    bufmgr_flush_all(buf);
    bufmgr_destroy(buf);
    return EXIT_SUCCESS;
  }

  while(true) {
    print_prompt();
    Node* n = parse_sql();

    if (n == NULL) continue;

    print_node(n);

    Query* qt = analyze_parsetree(buf, n);

    switch (qt->stmt) {
      case STMT_SYSCMD:
        if (parse_syscmd(((SysCmd*)n)->cmd) == SYSCMD_QUIT) {
          free_node(n);
          free_querytree(qt);
          printf("Shutting down...\n");
          bufmgr_flush_all(buf);
          bufmgr_destroy(buf);
          return EXIT_SUCCESS;
        } else {
          run_syscmd(((SysCmd*)n)->cmd, buf);
        }
        break;
      case STMT_SELECT:
        // run a select statement (single table only for now)
        // get a table descriptor with only the columns needed
        TableDesc *td = new_tabledesc(qt->tableList->)
        break;
      case STMT_ERROR:
        printf("semantic analysis failed\n");
        printf("Error: %s\n", qt->errorMessage);
        break;
    }

    free_node(n);
    free_querytree(qt);
  }

  return EXIT_SUCCESS;
}