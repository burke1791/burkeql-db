#ifndef RECORDSET_H
#define RECORDSET_H

#include "storage/record.h"
#include "utility/linkedlist.h"

typedef struct RecordSetRow {
  Datum* values;
  bool* isnull;
} RecordSetRow;

typedef struct RecordSet {
  LinkedList* rows;
} RecordSet;

RecordSet* new_recordset();
void free_recordset(RecordSet* rs, RecordDescriptor* rd);

RecordSetRow* new_recordset_row(LinkedList* rows, int ncols);
void free_recordset_row(RecordSetRow* row, RecordDescriptor* rd);

#endif /* RECORDSET_H */