#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "global/config.h"
#include "storage/page.h"

extern Config* conf;

Page new_page() {
  Page pg = malloc(conf->pageSize);
  memset(pg, 0, conf->pageSize);
  return pg;
}

void free_page(Page pg) {
  if (pg != NULL) free(pg);
}

void page_zero(Page pg) {
  memset(pg, 0, conf->pageSize);
}

void pageheader_set_pageid(Page pg, uint32_t pageId) {
  PageHeader* pgHdr = (PageHeader*)pg;
  pgHdr->pageId = pageId;
}

void pageheader_set_prevpageid(Page pg, uint32_t pageId) {
  PageHeader* pgHdr = (PageHeader*)pg;
  pgHdr->prevPageId = pageId;
}

void pageheader_set_nextpageid(Page pg, uint32_t pageId) {
  PageHeader* pgHdr = (PageHeader*)pg;
  pgHdr->nextPageId = pageId;
}

static bool page_has_space(Page pg, int length) {
  int availableSpace = ((PageHeader*)pg)->freeData;
  return availableSpace >= length;
}

/**
 * In order to insert a record on a page, we first need to determine
 * if there is enough continuous space on the page. For that, we simply
 * need to compare the length parameter (+ 4-bytes for the new SlotPointer)
 * against the `freeData` header field.
 * 
 * If there is sufficient space, we can `memcpy` the data parameter to the
 * correct spot on the page, then prepend our new SlotPointer to the slot
 * array.
 * 
 * Lastly, we update the header fields to reflect the new data and return
 * true.
 * 
 * If any of the above is not possible, we return false.
 */
bool page_insert(Page pg, Record data, uint16_t length) {
  int spaceRequired = length + sizeof(SlotPointer);
  if (!page_has_space(pg, spaceRequired)) return false;

  SlotPointer* sp = malloc(sizeof(SlotPointer));
  sp->length = length;

  /**
   * Calculating the new record's offset position:
   * PAGE_SIZE -
   * SLOT_ARRAY_SIZE -
   * `freeData`
   */
  int slotArraySize = ((PageHeader*)pg)->numRecords * sizeof(SlotPointer);
  sp->offset = conf->pageSize - slotArraySize - ((PageHeader*)pg)->freeData;

  /* copy the record data to the correct spot on the page */
  memcpy(pg + sp->offset, data, length);

  /* prepend the new slot pointer to the slot array */
  int newSlotOffset = conf->pageSize - slotArraySize - sizeof(SlotPointer);
  memcpy(pg + newSlotOffset, sp, sizeof(SlotPointer));

  /* update header fields */
  PageHeader* pgHdr = (PageHeader*)pg;
  pgHdr->numRecords++;
  pgHdr->freeBytes -= spaceRequired;
  pgHdr->freeData = conf->pageSize - (slotArraySize + sizeof(SlotPointer)) - (sp->offset + length);

  free(sp);

  return true;
}