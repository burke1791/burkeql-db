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
  BufPoolSlot* slots;
} BufPool;

BufPool* bufpool_init(FileDesc* fdesc, int numSlots);
void bufpool_destroy(BufPool* bp);

void bufpool_load_page(BufPool* bp, uint32_t pageId);
void bufpool_flush_page(BufPool* bp, uint32_t pageId);

#endif /* BUFPOOL_H */