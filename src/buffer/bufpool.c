#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "buffer/bufpool.h"
#include "global/config.h"
#include "storage/page.h"

extern Config* conf;

BufPool* bufpool_init(int size) {
  BufPool* bp = malloc(sizeof(BufPool));
  bp->size = size;
  bp->slots = calloc(size, sizeof(BufPoolSlot));

  // initialize each slot with a blank page
  for (int i = 0; i < size; i++) {
    bp->slots[i].pg = new_page();
    bp->slots[i].pageId = 0;
  }

  return bp;
}

void bufpool_destroy(BufPool* bp) {
  if (bp == NULL) return;

  if (bp->slots != NULL) {
    for (int i = 0; i < bp->size; i++) {
      if (bp->slots[i].pg != NULL) free_page(bp->slots[i].pg);
    }

    free(bp->slots);
  }

  free(bp);
}

static BufPoolSlot* bufpool_find_empty_slot(BufPool* bp) {
  for (int i = 0; i < bp->size; i++) {
    if (bp->slots[i].pageId == 0) return &bp->slots[i];
  }

  return NULL;
}

static BufPoolSlot* bufpool_find_slot_by_page_id(BufPool* bp, uint32_t pageId) {
  for (int i = 0; i < bp->size; i++) {
    if (bp->slots[i].pageId == pageId) return &bp->slots[i];
  }

  return NULL;
}

// static BufPoolSlot* bufpool_evict_page(BufPool* bp) {
//   // just evict the first page for now
//   uint32_t pageId = bp->slots[0].pageId;
//   bufpool_flush_page(bp, pageId);
//   bp->slots[0].pageId = 0;
//   return &bp->slots[0];
// }

bool bufpool_read_page(int fd, BufPool* bp, int32_t bufId, uint32_t pageId) {
  BufPoolSlot* slot = &bp->slots[bufId];

  lseek(fd, (pageId - 1) * conf->pageSize, SEEK_SET);
  int bytes_read = read(fd, slot->pg, conf->pageSize);

  if (bytes_read != conf->pageSize) {
    printf("Bytes read: %d\n", bytes_read);
    return false;
  }

  slot->pageId = pageId;

  return true;
}

BufPoolSlot* bufpool_new_page(BufPool* bp, uint32_t pageId) {
  BufPoolSlot* slot = bufpool_find_empty_slot(bp);

  slot->pageId = pageId;

  memset(slot->pg, 0, conf->pageSize);

  PageHeader* pgHdr = ((PageHeader*)slot->pg);
  pgHdr->pageId = slot->pageId;
  pgHdr->freeBytes = conf->pageSize - sizeof(PageHeader);
  pgHdr->freeData = conf->pageSize - sizeof(PageHeader);

  return slot;
}

static bool flush_page(int fd, Page pg, uint32_t pageId) {
  uint32_t headerPageId = ((PageHeader*)pg)->pageId;

  if (headerPageId != pageId || pageId == 0) {
    printf("Page Ids do not match\n");
    return false;
  }

  lseek(fd, (pageId - 1) * conf->pageSize, SEEK_SET);
  int bytes_written = write(fd, pg, conf->pageSize);

  if (bytes_written != conf->pageSize) {
    printf("Bytes written: %d\n", bytes_written);
    return false;
  }

  return true;
}

void bufpool_flush_page(int fd, BufPool* bp, uint32_t pageId) {
  BufPoolSlot* slot = bufpool_find_slot_by_page_id(bp, pageId);

  if (slot == NULL) {
    printf("Page does not exist in the buffer pool\n");
    return;
  }

  if (!flush_page(fd, slot->pg, pageId)) {
    printf("Flush page failure\n");
    return;
  }

  slot->pageId = 0;
}

void bufpool_flush_all(int fd, BufPool* bp) {
  for (int i = 0; i < bp->size; i++) {
    BufPoolSlot* slot = &bp->slots[i];
    flush_page(fd, slot->pg, slot->pageId);
  }
}