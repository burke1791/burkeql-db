#ifndef TABLEAM_H
#define TABLEAM_H

#include "storage/table.h"
#include "parser/parsetree.h"

void scan_table(TableDesc* td, ParseList* targetList, Datum* values);

#endif /* TABLEAM_H */