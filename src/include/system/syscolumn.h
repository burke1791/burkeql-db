#ifndef SYSCOLUMN_H
#define SYSCOLUMN_H

#include <stdint.h>

#include "storage/record.h"
#include "buffer/bufmgr.h"

typedef struct SysColumn {
  int64_t objectId;
  int64_t tableId;
  char* name;
  DataType dataType;
  int16_t maxLen;
  uint8_t precision;
  uint8_t scale;
  uint8_t colnum;
  uint8_t isNotNull;
} SysColumn;

RecordDescriptor* syscolumn_get_record_desc();

bool syscolumninit_insert_record(BufMgr* buf, SysColumn* c);

#endif /* SYSCOLUMN_H */