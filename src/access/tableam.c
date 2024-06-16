#include <stdlib.h>

#include "access/tableam.h"
#include "global/config.h"
#include "system/systable.h"

extern Config* conf;

/**
 * @brief 
 * 
 * @param buf 
 * @param td 
 * @param rs 
 */
void tableam_fullscan(BufMgr* buf, TableDesc* td, RecordSet* rs) {
  int32_t pageId = systable_get_first_pageid(buf, td->tablename);
  BufTag* tag = bufdesc_new_buftag(FILE_DATA, pageId);
  int32_t bufId = bufmgr_request_bufId(buf, tag);

  while (bufId >= 0) {
    Page pg = (Page)buf->bp->pages[bufId];
    PageHeader* pgHdr = (PageHeader*)pg;
    int numRecords = pgHdr->numRecords;

    for (int i = 0; i < numRecords; i++) {
      RecordSetRow* row = new_recordset_row(rs->rows, td->rd->ncols);
      int slotPointerOffset = conf->pageSize - (sizeof(SlotPointer) * (i + 1));
      SlotPointer* sp = (SlotPointer*)(pg + slotPointerOffset);
      defill_record(td->rd, pg + sp->offset, row->values, row->isnull);
    }

    tag->pageId = pgHdr->nextPageId;
    bufmgr_release_bufId(buf, bufId);
    bufId = bufmgr_request_bufId(buf, tag);
  }

  bufdesc_free_buftag(tag);
}

/**
 * @brief Inserts a record into a table
 * 
 * Loops through all pages until it finds enough empty space to insert the record
 * 
 * @hardcode - we're hard-coding pageId = 1 for now
 * 
 * @param buf 
 * @param td (currently unused)
 * @param values 
 * @return true 
 * @return false 
 */
bool tableam_insert(BufMgr* buf, TableDesc* td, Record r, uint16_t recordLen) {
  BufTag* tag = malloc(sizeof(BufTag));
  tag->fileId = FILE_DATA;
  tag->pageId = 1;
  int32_t bufId = bufmgr_request_bufId(buf, tag);

  if (bufId < 0) return false;

  while (bufId >= 0) {
    Page pg = buf->bp->pages[bufId];
    if (page_insert(pg, r, recordLen)) {
      free(tag);
      return true;
    }

    tag->pageId = ((PageHeader*)pg)->nextPageId;
    bufId = bufmgr_request_bufId(buf, tag);
  }

  free(tag);

  return false;
}