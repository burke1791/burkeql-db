#ifndef SYSSEQUENCE_H
#define SYSSEQUENCE_H

#include <stdint.h>

#include "buffer/bufmgr.h"
#include "storage/record.h"

typedef struct SysSequence {
  int64_t objectId;
  char* name;
  char* type;     /* `s` for system sequences, `u` for user-created sequences */
  int64_t columnId;
  int64_t nextValue;
  int64_t increment;
} SysSequence;

RecordDescriptor* syssequence_get_record_desc();

bool syssequenceinit_insert_record(BufMgr* buf, SysSequence* s);

#endif /* SYSSEQUENCE_H */