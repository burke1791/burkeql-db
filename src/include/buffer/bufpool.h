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

#include "storage/page.h"
#include "buffer/bufdesc.h"
#include "buffer/buffile.h"

typedef struct BufPool {
  int size;
  Page* pages;
} BufPool;

BufPool* bufpool_init(int size);
void bufpool_destroy(BufPool* bp);

bool bufpool_read_page(FileDescList* fdl, BufPool* bp, int32_t bufId, BufTag* tag);
bool bufpool_flush_page(FileDescList* fdl, BufDescArr* bd, BufPool* bp, int32_t bufId);

#endif /* BUFPOOL_H */