#ifndef TABLEAM_H
#define TABLEAM_H

#include "storage/table.h"
#include "buffer/bufmgr.h"
#include "utility/linkedlist.h"
#include "parser/parsetree.h"  // remove this when there's no dependency on ParseList*

void tableam_fullscan(BufMgr* buf, TableDesc* td, LinkedList* rows);
bool tableam_insert(BufMgr* buf, TableDesc* td, Record r, uint16_t recordLen);

#endif /* TABLEAM_H */