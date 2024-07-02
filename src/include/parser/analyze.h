#ifndef ANALYZE_H
#define ANALYZE_H

#include "parser/parsetree.h"
#include "buffer/bufmgr.h"

bool analyze_parsetree(BufMgr* buf, Node* tree);

#endif /* ANALYZE_H */