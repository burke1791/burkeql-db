#ifndef SYSTABLE_H
#define SYSTABLE_H

#include <stdint.h>

#include "storage/record.h"
#include "buffer/bufmgr.h"

#define SYSTABLE_FIRST_PAGE_ID 2

typedef struct SysTable {
  int64_t objectId;
  char* name;
  char* type;           /* 's' for system | 'u' for user */
  int32_t firstPageId;
  int32_t lastPageId;
} SysTable;

RecordDescriptor* systable_get_record_desc();

bool systableinit_insert_record(BufMgr* buf, SysTable* t);

int64_t systable_get_objectId(BufMgr* buf, char* tablename);
int32_t systable_get_first_pageid(BufMgr* buf, char* tablename);
int32_t systable_get_last_pageid(BufMgr* buf, char* tablename);

bool systable_set_first_pageid(BufMgr* buf, char* tablename, int32_t firstPageId);
bool systable_set_last_pageid(BufMgr* buf, char* tablename, int32_t lastPageId);

#endif /* SYSTABLE_H */