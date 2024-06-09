#ifndef SYSOBJECT_H
#define SYSOBJECT_H

#include <stdlib.h>
#include <stdint.h>

#include "buffer/bufmgr.h"
#include "storage/datum.h"
#include "storage/record.h"

typedef struct SysObject {
  int64_t objectId;
  char* name;
  char* type;
} SysObject;

RecordDescriptor* sysobject_get_record_desc();

bool sysobjectinit_insert_record(BufMgr* buf, BufTag* tag, SysObject* obj);

#endif /* SYSOBJECT_H */
