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

#include "storage/file.h"
#include "buffer/bufpool.h"
#include "buffer/bufdesc.h"

typedef struct BufGlobal {
  uint32_t nextPageId;
} BufGlobal;

typedef struct BufMgr {
  FileDesc* fdesc;
  int size;
  BufGlobal* global;
  BufDescArr* bd;
  BufPool* bp;
} BufMgr;

BufMgr* bufmgr_init(FileDesc* fdesc);
void bufmgr_destroy(BufMgr* buf);

int32_t bufmgr_request_bufId(BufMgr* buf, uint32_t pageId);

/**
 * @brief System command functions for printing diagnostic information to the terminal
 * 
 */

void bufmgr_diag_summary(BufMgr* buf);

#endif /* BUFMGR_H */