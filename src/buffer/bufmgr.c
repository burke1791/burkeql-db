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

static int32_t bufmgr_evict_page(BufDescArr* bd) {
  printf("page eviction logic needed\n");
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
    bufId = bufmgr_evict_page(buf->bd);
    return -1;
  }

  if (!bufpool_read_page(buf->fdl, buf->bp, bufId, tag)) {
    printf("Unable to read fileId: %d, pageId: %d\n", tag->fileId, tag->pageId);
    return -1;
  }

  bufdesc_set_tag(buf->bd->descArr[bufId], tag);
  bufdesc_end_io(buf->bd->descArr[bufId]);
  bufdesc_pin(buf->bd->descArr[bufId]);

  return bufId;
}

/**
 * @brief returns the buffer_id of the requested page. If the page is not in memory,
 * then we read it from disk and store it in an unoccupied slot - possibly
 * requiring the eviction of an unused page.
 * 
 * First, we loop through the BufDescArr to see if the requested BufTag is present.
 * If so, we pin the BufDesc and return the index array of the BufDesc (i.e. the buffer_id)
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

  return bufId;
}

int32_t bufmgr_allocate_new_page(BufMgr* buf, uint32_t fileId) {
  uint32_t pageId = buffile_get_new_pageid(buf->fdl, fileId);

  // claim a slot in the buffer pool
  int32_t bufId = bufdesc_find_empty_slot(buf->bd);

  if (bufId < 0) {
    bufId = bufmgr_evict_page(buf->bd);
  }

  bufdesc_start_io(buf->bd->descArr[bufId]);
  bufdesc_pin(buf->bd->descArr[bufId]);
  buf->bd->descArr[bufId]->tag->fileId = fileId;
  buf->bd->descArr[bufId]->tag->pageId = pageId;

  pageheader_set_pageid(buf->bp->pages[bufId], pageId);

  return bufId;
}

/**
 * @brief Allocates a new page at the end of a linkedlist of pages. Caller
 * is responsible for updating the `lastPageId` field in the system table.
 * 
 * @details The buffer manager will first request the next pageId from
 * the buffer file layer, then initialize the page with the appropriate
 * header fields.
 * 
 * In this "append" case, the `nextPageId` header field will be set to zero,
 * while the `prevPageId` field is set to the pageId of the page represented
 * by the `prev` BufTag function param.
 * 
 * @param buf 
 * @param prev 
 * @return int32_t 
 */
int32_t bufmgr_pagesplit_append(BufMgr* buf, BufTag* prev) {
  int32_t newBufId = bufmgr_allocate_new_page(buf, FILE_DATA);
  int32_t newPageId = ((PageHeader*)buf->bp->pages[newBufId])->pageId;

  pageheader_set_prevpageid(buf->bp->pages[newBufId], prev->pageId);

  uint32_t oldBufId = bufmgr_request_bufId(buf, prev);
  pageheader_set_nextpageid(buf->bp->pages[oldBufId], newPageId);

  /**
   * Need to make sure we pull the new page back into memory in case it
   * was evicted by the above
  */
  BufTag* tag = bufdesc_new_buftag(FILE_DATA, newPageId);
  newBufId = bufmgr_request_bufId(buf, tag);

  bufdesc_free_buftag(tag);

  return newBufId;
}

void bufmgr_flush_page(BufMgr* buf, BufTag* tag) {
  int32_t bufId = bufmgr_request_bufId(buf, tag);
  bufpool_flush_page(buf->fdl, buf->bd, buf->bp, bufId);
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