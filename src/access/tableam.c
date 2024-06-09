#include <stdlib.h>

#include "access/tableam.h"
#include "resultset/recordset.h"
#include "global/config.h"

extern Config* conf;

/**
 * scan_table
 * 
 * Starting with pageId 1, we loop through all slot pointers.
 * 1. Allocate a Datum array
 * 2. `defill_record` the Datum array only with the columns in RecordSet->RecordDescriptor
 * 3. Append the Datum array to RecordSet->rows
 * 4. Repeat until the PgHdr->nextPageId = 0
 */
void tableam_fullscan(BufMgr* buf, TableDesc* td, LinkedList* rows) {
  BufTag* tag = malloc(sizeof(BufTag));
  tag->fileId = FILE_DATA;
  tag->pageId = 1;
  int32_t bufId = bufmgr_request_bufId(buf, tag);

  while (bufId >= 0) {
    Page pg = (Page)buf->bp->pages[bufId];
    PageHeader* pgHdr = (PageHeader*)pg;
    int numRecords = pgHdr->numRecords;

    for (int i = 0; i < numRecords; i++) {
      RecordSetRow* row = new_recordset_row(td->rd->ncols);
      int slotPointerOffset = conf->pageSize - (sizeof(SlotPointer) * (i + 1));
      SlotPointer* sp = (SlotPointer*)(pg + slotPointerOffset);
      defill_record(td->rd, pg + sp->offset, row->values, row->isnull);
      linkedlist_append(rows, row);
    }

    tag->pageId = pgHdr->nextPageId;
    bufId = bufmgr_request_bufId(buf, tag);
  }

  free(tag);
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