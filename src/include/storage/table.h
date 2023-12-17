#ifndef TABLE_H
#define TABLE_H

#include "storage/record.h"

typedef struct TableDesc {
  char* tablename;
  RecordDescriptor* rd;
} TableDesc;

#endif /* TABLE_H */