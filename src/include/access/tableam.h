#ifndef TABLEAM_H
#define TABLEAM_H

#include "storage/table.h"
#include "buffer/bufpool.h"
#include "utility/linkedlist.h"

void tableam_fullscan(BufPool* bp, TableDesc* td, LinkedList* rows);

#endif /* TABLEAM_H */