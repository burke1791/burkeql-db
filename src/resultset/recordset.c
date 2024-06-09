#include <stdlib.h>

#include "resultset/recordset.h"
#include "utility/linkedlist.h"

RecordSet* new_recordset() {
  RecordSet* rs = malloc(sizeof(RecordSet));
  rs->rows = NULL;

  return rs;
}

static void free_recordset_row_columns(RecordSetRow* row, RecordDescriptor* rd) {
  for (int i = 0; i < rd->ncols; i++) {
    if (rd->cols[i].dataType == DT_CHAR || rd->cols[i].dataType == DT_VARCHAR) {
      free((void*)row->values[i]);
    }
  }
}

static void free_recordset_row_list(LinkedList* rows, RecordDescriptor* rd) {
  ListItem* li = rows->head;

  while (li != NULL) {
    free_recordset_row((RecordSetRow*)li->ptr, rd);
    li = li->next;
  }
}

void free_recordset(RecordSet* rs, RecordDescriptor* rd) {
  if (rs == NULL) return;

  if (rs->rows != NULL) {
    free_recordset_row_list(rs->rows, rd);
    free_linkedlist(rs->rows, NULL);
  }

  free(rs);
}

RecordSetRow* new_recordset_row(int ncols) {
  RecordSetRow* row = malloc(sizeof(RecordSetRow));
  row->values = malloc(ncols * sizeof(Datum));
  row->isnull = malloc(ncols * sizeof(bool));
  return row;
}

void free_recordset_row(RecordSetRow* row, RecordDescriptor* rd) {
  if (row->values != NULL) {
    free_recordset_row_columns(row, rd);
    free(row->values);
    free(row->isnull);
  }
  free(row);
}