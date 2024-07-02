#include <stdlib.h>

#include "resultset/recordset.h"
#include "storage/table.h"
#include "access/tableam.h"



TableDesc* new_tabledesc(char* tablename) {
  TableDesc* td = malloc(sizeof(TableDesc));
  td->tablename = tablename;
  td->rd = NULL;

  return td;
}

void free_tabledesc(TableDesc* td) {
  if (td == NULL) return;

  // if (td->tablename != NULL) free(td->tablename);
  if (td->rd != NULL) free_record_desc(td->rd);
  free(td);
}