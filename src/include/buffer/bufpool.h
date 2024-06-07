/**
 * @file bufpool.h
 * @author Chris Burke
 * @brief Buffer pool API
 * @version 0.1
 * @date 2024-06-04
 * 
 * @copyright Copyright (c) 2024
 * 
 */

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
  int size;
  BufPoolSlot* slots;
} BufPool;

BufPool* bufpool_init(int size);
void bufpool_destroy(BufPool* bp);

bool bufpool_read_page(int fd, BufPool* bp, int32_t bufId, uint32_t pageId);
BufPoolSlot* bufpool_new_page(BufPool* bp, uint32_t pageId);
void bufpool_flush_page(int fd, BufPool* bp, uint32_t pageId);
void bufpool_flush_all(int fd, BufPool* bp);

#endif /* BUFPOOL_H */