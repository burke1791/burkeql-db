#include <stdlib.h>

#include "resultset/recordset.h"

RecordSet* new_recordset() {
  RecordSet* rs = malloc(sizeof(RecordSet));
  rs->rd = NULL;
  rs->rows = new_linkedlist();

  return rs;
}

void free_recordset(RecordSet* rs) {
  if (rs == NULL) return;

  if (rs->rd != NULL) free_record_desc(rs->rd);
  if (rs->rows != NULL) free_linkedlist(rs->rows);
}