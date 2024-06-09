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
int32_t bufmgr_allocate_new_page(BufMgr* buf, uint32_t fileId);

/**
 * @brief Allocates a new page at the end of a linkedlist of pages
 * 
 * @param buf 
 * @param prev 
 * @return int32_t 
 */
int32_t bufmgr_pagesplit_append(BufMgr* buf, BufTag* prev);

/**
 * @brief Allocates a new page in the middle of a linked list of pages
 * 
 * @param buf 
 * @param prev 
 * @param next 
 * @return int32_t 
 */
int32_t bufmgr_pagesplit_insert(BufMgr* buf, BufTag* prev, BufTag* next);

void bufmgr_flush_page(BufMgr* buf, BufTag* tag);
void bufmgr_flush_all(BufMgr* buf);

/**
 * @brief System command functions for printing diagnostic information to the terminal
 * 
 */

void bufmgr_diag_summary(BufMgr* buf);

#endif /* BUFMGR_H */