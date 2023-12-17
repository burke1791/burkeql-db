#ifndef TABLE_H
#define TABLE_H

#include "storage/record.h"

typedef struct TableDesc {
  char* tablename;
  RecordDescriptor* rd;
} TableDesc;

TableDesc* new_tabledesc(const char* tablename);

void free_tabledesc(TableDesc* td);

#endif /* TABLE_H */