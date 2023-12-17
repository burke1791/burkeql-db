#ifndef RECORDSET_H
#define RECORDSET_H

#include "storage/record.h"
#include "utility/linkedlist.h"

typedef struct RecordSet {
  RecordDescriptor* rd;
  LinkedList* rows;
} RecordSet;

RecordSet* new_recordset();
void free_recordset(RecordSet* rs);

#endif /* RECORDSET_H */