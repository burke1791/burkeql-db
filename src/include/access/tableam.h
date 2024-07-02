#ifndef TABLEAM_H
#define TABLEAM_H

#include "storage/table.h"
#include "buffer/bufmgr.h"
#include "utility/linkedlist.h"
#include "resultset/recordset.h"
#include "access/access.h"

void tableam_fullscan(BufMgr* buf, TableDesc* td, RecordSet* rs);
bool tableam_insert(BufMgr* buf, TableDesc* td, Record r, uint16_t recordLen);

void tableam_filterscan(BufMgr* buf, TableDesc* td, Filter* filter, RecordSet* rs);

#endif /* TABLEAM_H */