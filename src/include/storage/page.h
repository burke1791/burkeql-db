#ifndef PAGE_H
#define PAGE_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "storage/record.h"

typedef char* Page;

/**
 * @brief the 20-byte data page header structure
 * 
 * pageId     | one-based page identifier, numbered sequentially from the beginning of
 *              the file to the end. Gaps are not allowed
 * pageType   | 0=data page, 1=index page
 * indexLevel | level of the page in a B+tree index. 0=leaf level (or heap page)
 * prevPageId | pointer to the previous page in the current table or index at the
 *              same index level
 * nextPageId | pointer to the next page in the current table or index at the same
 *              index level
 * numRecords | count of the slot array entries
 * freeBytes  | number of free bytes on the page
 * freeData   | number of continuous free bytes between the last record and the
 *              beginning of the slot array
 * 
 */
#pragma pack(push, 1) /* disabling memory alignment because I don't want to deal with it */
typedef struct PageHeader {
  uint32_t pageId;
  uint8_t pageType;
  uint8_t indexLevel;
  uint32_t prevPageId;
  uint32_t nextPageId;
  uint16_t numRecords;
  uint16_t freeBytes;
  uint16_t freeData;
} PageHeader;

typedef struct SlotPointer {
  uint16_t offset;
  uint16_t length;
} SlotPointer;

Page new_page();
void free_page(Page pg);

void page_zero(Page pg);
void pageheader_init_datapage(Page pg);

void pageheader_set_pageid(Page pg, uint32_t pageId);
void pageheader_set_prevpageid(Page pg, uint32_t pageId);
void pageheader_set_nextpageid(Page pg, uint32_t pageId);

bool page_insert(Page pg, Record data, uint16_t length);

#endif /* PAGE_H */