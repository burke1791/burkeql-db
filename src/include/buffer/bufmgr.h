/**
 * @file bufmgr.h
 * @author Chris Burke
 * @brief 
 * @version 0.1
 * @date 2024-06-04
 * 
 * @copyright Copyright (c) 2024
 * 
 * The buffer manager is BurkeQL's "memory" where we store all data and index pages
 * currently in use by the system. It has two primary layers:
 *    - buffer descriptors
 *    - buffer pool
 * 
 * The buffer pool layer is where the actual data pages are stored as an array.
 * 
 * The buffer descriptors layer is an array with a one-to-one relationship to entries
 * in the buffer pool. The descriptors are where we store metadata about the associated
 * page in the buffer pool.
 * 
 * 
 * Common buffer manager workflows:
 *    - accessing a page already in the buffer pool
 *    - loading a page from storage into an empty slot
 *    - loading a page from storage and choosing as existing page to evict
 * 
 */

#ifndef BUFMGR_H
#define BUFMGR_H

#include <stdbool.h>

#include "buffer/bufpool.h"
#include "buffer/bufdesc.h"
#include "buffer/buffile.h"

typedef struct BufGlobal {
  int64_t nextObjectId;
} BufGlobal;

typedef struct BufMgr {
  FileDescList* fdl;
  int size;
  BufGlobal* global;
  BufDescArr* bd;
  BufPool* bp;
} BufMgr;

BufMgr* bufmgr_init();
void bufmgr_destroy(BufMgr* buf);

int32_t bufmgr_request_bufId(BufMgr* buf, BufTag* tag);
void bufmgr_release_bufId(BufMgr* buf, int32_t bufId);
int32_t bufmgr_allocate_new_page(BufMgr* buf, uint32_t fileId);

void bufmgr_flush_page(BufMgr* buf, BufTag* tag);
void bufmgr_flush_all(BufMgr* buf);

/**
 * @brief Splits a database page. The page represented by `tag`
 * will be the "prevPageId" for the new page.
 * 
 * @details This function allocates a new page for the table or index
 * represented by `objectId`. It then synchronizes the linked list
 * of pageId's in the page header. If this is an "append" page split, 
 * then we only update the previous page's header fields. If this is an
 * "insert" page split, then we need to update both the previous page
 * and the next page.
 * 
 * @param buf 
 * @param tag 
 * @return int32_t
 */
int32_t bufmgr_page_split(BufMgr* buf, int32_t bufId);
/* Same as the above, but it does not synchronize the `lastPageId` column */
int32_t bufmgrinit_page_split(BufMgr* buf, BufTag* tag);

/**
 * @brief System command functions for printing diagnostic information to the terminal
 * 
 */

void bufmgr_diag_summary(BufMgr* buf);
void bufmgr_diag_details(BufMgr* buf);

#endif /* BUFMGR_H */