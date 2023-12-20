#ifndef BUFPOOL_H
#define BUFPOOL_H

#include <stdbool.h>

#include "storage/file.h"
#include "storage/page.h"

typedef struct BufPoolSlot {
  uint32_t pageId;
  Page pg;
} BufPoolSlot;

typedef struct BufPool {
  FileDesc* fdesc;
  int size;
  uint32_t nextPageId;  // the next available pageId
  BufPoolSlot* slots;
} BufPool;

BufPool* bufpool_init(FileDesc* fdesc, int numSlots);
void bufpool_destroy(BufPool* bp);

BufPoolSlot* bufpool_read_page(BufPool* bp, uint32_t pageId);
BufPoolSlot* bufpool_new_page(BufPool* bp);
void bufpool_flush_page(BufPool* bp, uint32_t pageId);
void bufpool_flush_all(BufPool* bp);

#endif /* BUFPOOL_H */