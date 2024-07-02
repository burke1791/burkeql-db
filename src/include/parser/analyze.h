#ifndef ANALYZE_H
#define ANALYZE_H

#include "parser/querytree.h"
#include "buffer/bufmgr.h"

Query* analyze_parsetree(BufMgr* buf, Node* tree);

#endif /* ANALYZE_H */