#include <unistd.h>
#include <stdlib.h>

#include "buffer/bufmgr.h"
#include "global/config.h"

extern Config* conf;

static BufGlobal* bufmgr_globals_init(FileDesc* fdesc) {
  BufGlobal* g = malloc(sizeof(BufGlobal));

  int offset = lseek(fdesc->fd, -(conf->pageSize), SEEK_END);

  if (offset == -1) {
    g->nextPageId = 1;
  } else {
    g->nextPageId = (offset / conf->pageSize) + 1;
  }

  return g;
}

static void bufmgr_globals_destroy(BufGlobal* g) {
  free(g);
}

BufMgr* bufmgr_init(FileDesc* fdesc) {
  BufMgr* buf = malloc(sizeof(BufMgr));

  buf->fdesc = fdesc;
  buf->size = conf->bufpoolSize;
  buf->global = bufmgr_globals_init(fdesc);
  buf->bd = bufdesc_init(buf->size);
  buf->bp = bufpool_init(buf->size);
}

void bufmgr_destroy(BufMgr* buf) {
  bufmgr_globals_destroy(buf->global);
  bufdesc_destroy(buf->bd);
  bufpool_destroy(buf->bp);

  free(buf);
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
static int32_t bufmgr_load_page(BufMgr* buf, uint32_t pageId) {
  int32_t bufId = bufdesc_find_empty_slot(buf->bd);

  if (bufId < 0) {
    // evict an unused page then proceed
    printf("page eviction logic needed\n");
    return -1;
  }

  if (!bufpool_read_page(buf->fdesc->fd, buf->bp, bufId, pageId)) {
    printf("Unable to read pageId: %d\n", pageId);
    return -1;
  }

  buf->bd->descArr[bufId]->pageId = pageId;

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
int32_t bufmgr_request_bufId(BufMgr* buf, uint32_t pageId) {
  if (pageId <= 0) return -1;

  int32_t bufId = bufdesc_find_slot(buf->bd, pageId);

  if (bufId < 0) {
    bufId = bufmgr_load_page(buf, pageId);
  }

  return bufId;
}


void bufmgr_diag_summary(BufMgr* buf) {
  printf("----------------------------------\n");
  printf("---   Buffer Manager Summary   ---\n");
  printf("----------------------------------\n");
  printf("Cache Size: %d\n", buf->size);

  int pagesInCache = 0;
  for (int i = 0; i < buf->size; i++) {
    if (buf->bd->descArr[i]->pageId > 0) pagesInCache++;
  }

  printf("Pages In Cache: %d\n", pagesInCache);
  printf("----------------------------------\n");
}