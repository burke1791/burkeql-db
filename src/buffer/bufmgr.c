#include <unistd.h>
#include <stdlib.h>

#include "buffer/bufmgr.h"
#include "global/config.h"

extern Config* conf;

static BufGlobal* bufmgr_globals_init() {
  BufGlobal* g = malloc(sizeof(BufGlobal));

  /**
   * @todo set globals
   */

  return g;
}

static void bufmgr_globals_destroy(BufGlobal* g) {
  free(g);
}

BufMgr* bufmgr_init() {
  BufMgr* buf = malloc(sizeof(BufMgr));

  buf->fdl = buffile_init();
  buf->size = conf->bufpoolSize;
  buf->global = bufmgr_globals_init();
  buf->bd = bufdesc_init(buf->size);
  buf->bp = bufpool_init(buf->size);
}

void bufmgr_destroy(BufMgr* buf) {
  bufmgr_globals_destroy(buf->global);
  bufdesc_destroy(buf->bd);
  bufpool_destroy(buf->bp);
  buffile_destroy(buf->fdl);

  free(buf);
}

/**
 * @brief Loops through the buffer descriptors array and
 * evicts the first unused page it finds. An unused page is
 * one that has pinCount = 0.
 * 
 * @param bd 
 * @return int32_t 
 */
static int32_t bufmgr_evict_page(BufMgr* buf) {
  for (int i = 0; i < buf->bd->size; i++) {
    BufDesc* bdesc = buf->bd->descArr[i];
    if (bdesc->pinCount <= 0) {
      bufdesc_start_io(bdesc);
      if (bufpool_flush_page(buf->fdl, buf->bd, buf->bp, i)) {
        bufdesc_reset(bdesc);
        return i;
      }
    }
  }
  printf("Unable to evict page\n");
  return -1;
}

/**
 * @brief Loads the requested page from disk and returns the associated buffer_id.
 * The caller is responsible for ensuring the requested page is not already in
 * the buffer pool
 * 
 * @param buf 
 * @param pageId 
 * @return int32_t 
 */
static int32_t bufmgr_load_page(BufMgr* buf, BufTag* tag) {
  int32_t bufId = bufdesc_find_empty_slot(buf->bd);

  if (bufId < 0) {
    bufId = bufmgr_evict_page(buf);
  }

  if (!bufpool_read_page(buf->fdl, buf->bp, bufId, tag)) {
    return -1;
  }

  bufdesc_set_tag(buf->bd->descArr[bufId], tag);
  bufdesc_end_io(buf->bd->descArr[bufId]);

  return bufId;
}

/**
 * @brief returns the buffer_id of the requested page. If the page is not in memory,
 * then we read it from disk and store it in an unoccupied slot - possibly
 * requiring the eviction of an unused page. This function pins the page - caller is
 * responsible for unpinning the page.
 * 
 * @details First, we loop through the BufDescArr to see if the requested BufTag is present.
 * If so, we pin the BufDesc and return the array index of the BufDesc (i.e. the buffer_id)
 * to the caller.
 * 
 * If the BufTag is not present, we search for an empty slot and load the page from disk.
 * 
 * If there is no empty slot, then we need to evict an unused page, then load the requested
 * page from disk.
 * 
 * @param buf 
 * @param tag 
 * @return int32_t 
 */
int32_t bufmgr_request_bufId(BufMgr* buf, BufTag* tag) {
  if (tag->pageId <= 0) return -1;

  int32_t bufId = bufdesc_find_slot(buf->bd, tag);

  if (bufId < 0) {
    bufId = bufmgr_load_page(buf, tag);
  }

  if (bufId >= 0) bufdesc_pin(buf->bd->descArr[bufId]);

  return bufId;
}

void bufmgr_release_bufId(BufMgr* buf, int32_t bufId) {
  bufdesc_unpin(buf->bd->descArr[bufId]);
}

int32_t bufmgr_allocate_new_page(BufMgr* buf, uint32_t fileId) {
  uint32_t pageId = buffile_get_new_pageid(buf->fdl, fileId);

  // claim a slot in the buffer pool
  int32_t bufId = bufdesc_find_empty_slot(buf->bd);

  if (bufId < 0) {
    bufId = bufmgr_evict_page(buf);
  }

  bufdesc_start_io(buf->bd->descArr[bufId]);
  bufdesc_pin(buf->bd->descArr[bufId]);
  buf->bd->descArr[bufId]->tag->fileId = fileId;
  buf->bd->descArr[bufId]->tag->pageId = pageId;

  page_zero(buf->bp->pages[bufId]);
  pageheader_set_pageid(buf->bp->pages[bufId], pageId);
  bufdesc_end_io(buf->bd->descArr[bufId]);

  return bufId;
}

void bufmgr_flush_page(BufMgr* buf, BufTag* tag) {
  int32_t bufId = bufmgr_request_bufId(buf, tag);
  bufpool_flush_page(buf->fdl, buf->bd, buf->bp, bufId);
  bufdesc_reset(buf->bd->descArr[bufId]);
}

/**
 * @brief Flushes all pages in memory to disk
 * 
 * Future optimization: only flush to disk if the page is dirty
 * 
 * @param buf 
 */
void bufmgr_flush_all(BufMgr* buf) {
  for (int i = 0; i < buf->size; i++) {
    bufpool_flush_page(buf->fdl, buf->bd, buf->bp, i);
  }
}

static int32_t bufmgrinit_page_split_append(BufMgr* buf, int32_t prevBufId) {
  int32_t bufId = bufmgr_allocate_new_page(buf, FILE_DATA);
  if (bufId < 0) return -1;
  pageheader_init_datapage(buf->bp->pages[bufId]);
  pageheader_set_prevpageid(buf->bp->pages[bufId], buf->bd->descArr[prevBufId]->tag->pageId);
  int32_t newPageId = buf->bd->descArr[bufId]->tag->pageId;

  pageheader_set_nextpageid(buf->bp->pages[prevBufId], newPageId);
  bufmgr_release_bufId(buf, prevBufId);

  return bufId;
}

/**
 * @brief Same as bufmgr_page_split, except we don't synchronize
 * the `lastPageId` column in the system tables because during
 * init, there might not be an entry for the object we're splitting
 * 
 * @param buf 
 * @param tag 
 * @return int32_t 
 */
int32_t bufmgrinit_page_split(BufMgr* buf, BufTag* tag) {
  int32_t bufId = bufmgr_request_bufId(buf, tag);
  if (bufId < 0) return -1;

  PageHeader* pgHdr = (PageHeader*)buf->bp->pages[bufId];

  if (pgHdr->nextPageId == 0) {
    bufId = bufmgrinit_page_split_append(buf, bufId);
  } else {
    printf("Implement an insert page split\n");
    return -1;
  }

  return bufId;
}

static int32_t bufmgr_page_split_append(BufMgr* buf, int32_t prevBufId) {
  int32_t bufId = bufmgr_allocate_new_page(buf, FILE_DATA);
  if (bufId < 0) return -1;
  pageheader_init_datapage(buf->bp->pages[bufId]);
  pageheader_set_prevpageid(buf->bp->pages[bufId], buf->bd->descArr[prevBufId]->tag->pageId);
  int32_t newPageId = buf->bd->descArr[bufId]->tag->pageId;

  pageheader_set_nextpageid(buf->bp->pages[prevBufId], newPageId);
  bufmgr_release_bufId(buf, prevBufId);

  return bufId;
}

/**
 * @brief Splits a database page. The page represented by `tag`
 * will be the "prevPageId" for the new page.
 * 
 * @details There are two types of page split operations: append-only and insert.
 * In order to determine which is required, we look at the `nextPageId` header
 * field of the data page represented by `tag`. If it's set to '0', then we
 * have an append-only page split. Any other value means we need to perform
 * an insert page split.
 * 
 * For the append-only case, we simply need to allocate a new page, then
 * update the old page's `nextPageId` header field with the new pageId.
 * And when setting the header fields for the new page, we make sure to
 * set `prevPageId` appropriately.
 * 
 * Lastly, we update the `lastPageId` column in the appropriate system table.
 * 
 * @todo Implement the "insert" page split code path. Including moving
 * the necessary data from the old page to the new page - to maintain
 * the sort-order of whatever key the data are physically sorted by.
 * 
 * @param buf 
 * @param tag 
 * @return int32_t 
 */
int32_t bufmgr_page_split(BufMgr* buf, int32_t bufId) {
  if (bufId < 0) return -1;

  PageHeader* pgHdr = (PageHeader*)buf->bp->pages[bufId];

  int32_t newBufId = -1;
  if (pgHdr->nextPageId == 0) {
    newBufId = bufmgr_page_split_append(buf, bufId);
  } else {
    printf("Implement an insert page split\n");
    return -1;
  }

  return newBufId;
}


void bufmgr_diag_summary(BufMgr* buf) {
  printf("----------------------------------\n");
  printf("---   Buffer Manager Summary   ---\n");
  printf("----------------------------------\n");
  printf("= Cache Size: %d\n", buf->size);

  int logPagesInCache = 0;
  int dataPagesInCache = 0;
  for (int i = 0; i < buf->size; i++) {
    if (buf->bd->descArr[i]->tag->pageId > 0) {
      if (buf->bd->descArr[i]->tag->fileId == FILE_DATA) dataPagesInCache++;
      if (buf->bd->descArr[i]->tag->fileId == FILE_LOG) logPagesInCache++;
    }
  }

  printf("\n");
  printf("= Total Pages In Cache: %d\n", dataPagesInCache + logPagesInCache);
  printf("=   Log Pages:          %d\n", logPagesInCache);
  printf("=   Data Pages:         %d\n", dataPagesInCache);
  printf("----------------------------------\n");
}

void bufmgr_diag_details(BufMgr* buf) {
  bufmgr_diag_summary(buf);

  printf("---   Buffer Manager Details   ---\n");
  printf("----------------------------------\n");

  for (int i = 0; i < buf->size; i++) {
    if (bufdesc_is_unused(buf->bd->descArr[i])) {
      printf("= PageId: N/A\n");
    } else {
      printf("= PageId: %d\n", buf->bd->descArr[i]->tag->pageId);
      printf("=  Dirty: %d | Valid: %d\n", buf->bd->descArr[i]->isDirty, buf->bd->descArr[i]->isValid);
      printf("=  Pins:  %d | Uses:  %d\n", buf->bd->descArr[i]->pinCount, buf->bd->descArr[i]->useCount);
    }
    printf("----------------------------------\n");
  }
}