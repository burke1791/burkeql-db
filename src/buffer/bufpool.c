#include <stdlib.h>

#include "buffer/bufpool.h"

BufPool* bufpool_init(FileDesc* fdesc, int numSlots) {
  BufPool* bp = malloc(sizeof(BufPool));
  bp->fdesc = fdesc;
  bp->size = numSlots;
  bp->slots = calloc(numSlots, sizeof(BufPoolSlot));

  // initialize each slot with a blank page
  for (int i = 0; i < numSlots; i++) {
    bp->slots[i].pg = new_page();
  }

  return bp;
}

void bufpool_destroy(BufPool* bp) {
  if (bp == NULL) return;

  if (bp->slots != NULL) {
    for (int i = 0; i < bp->size; i++) {
      if (bp->slots[i].pg != NULL) free(bp->slots[i].pg);
    }

    free(bp->slots);
  }

  free(bp);
}

static BufPoolSlot* find_empty_bufpoolslot(BufPool* bp) {
  for (int i = 0; i < bp->size; i++) {
    if (bp->slots[i].pageId == 0) return &bp->slots[i];
  }

  return NULL;
}

static BufPoolSlot* bufpool_evict_page(BufPool* bp) {
  // just evict the first page for now
  uint32_t pageId = bp->slots[0].pageId;
  bufpool_flush_page(bp, pageId);
  bp->slots[0].pageId = 0;
  return &bp->slots[0];
}

void bufpool_read_page(BufPool* bp, uint32_t pageId) {
  BufPoolSlot* slot = find_empty_bufpoolslot(bp);

  if (slot == NULL) {
    // evict a page and return the empty slot
    slot = bufpool_evict_page(bp);
  }

  
}

void bufpool_flush_page(BufPool* bp, uint32_t pageId) {

}