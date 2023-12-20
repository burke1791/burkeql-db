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
void tableam_fullscan(BufPool* bp, TableDesc* td, LinkedList* rows) {
  BufPoolSlot* slot = bufpool_read_page(bp, 1);

  while (slot != NULL) {
    PageHeader* pgHdr = (PageHeader*)slot->pg;
    int numRecords = pgHdr->numRecords;

    for (int i = 0; i < numRecords; i++) {
      RecordSetRow* row = new_recordset_row(td->rd->ncols);
      int slotPointerOffset = conf->pageSize - (sizeof(SlotPointer) * (i + 1));
      SlotPointer* sp = (SlotPointer*)(slot->pg + slotPointerOffset);
      defill_record(td->rd, slot->pg + sp->offset, row->values);
      linkedlist_append(rows, (void*)row);
    }

    slot = bufpool_read_page(bp, pgHdr->nextPageId);
  }
}