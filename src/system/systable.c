#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "global/config.h"
#include "system/systable.h"
#include "resultset/recordset.h"

extern Config* conf;

RecordDescriptor* systable_get_record_desc() {
  RecordDescriptor* rd = malloc(sizeof(RecordDescriptor) + (5 * sizeof(Column)));
  rd->ncols = 5;
  rd->nfixed = 4;
  rd->hasNullableColumns = false;

  construct_column_desc(&rd->cols[0], "object_id", DT_BIGINT, 0, 8, true);
  construct_column_desc(&rd->cols[1], "name", DT_VARCHAR, 1, 50, true);
  construct_column_desc(&rd->cols[2], "type", DT_CHAR, 2, 1, true);
  construct_column_desc(&rd->cols[3], "first_page_id", DT_INT, 3, 4, true);
  construct_column_desc(&rd->cols[4], "last_page_id", DT_INT, 4, 4, true);

  return rd;
}

static void systable_populate_values_arrays(
  Datum* fixed,
  bool* fixedNull,
  Datum* varlen,
  bool* varlenNull,
  SysTable* t
) {
  fixed[0] = int64GetDatum(t->objectId);
  fixedNull[0] = false;
  fixed[1] = charGetDatum(t->type);
  fixedNull[1] = false;
  fixed[2] = int32GetDatum(t->firstPageId);
  fixedNull[2] = false;
  fixed[3] = int32GetDatum(t->lastPageId);
  fixedNull[3] = false;

  varlen[0] = charGetDatum(t->name);
  varlenNull[0] = false;
}

bool systableinit_insert_record(BufMgr* buf, SysTable* t) {
  RecordDescriptor* rd = systable_get_record_desc();

  Datum* fixed = malloc(sizeof(Datum) * rd->nfixed);
  bool* fixedNull = malloc(sizeof(bool) * rd->nfixed);
  Datum* varlen = malloc(sizeof(Datum) * (rd->ncols - rd->nfixed));
  bool* varlenNull = malloc(sizeof(bool) * (rd->ncols - rd->nfixed));

  systable_populate_values_arrays(fixed, fixedNull, varlen, varlenNull, t);

  uint16_t recordLen = compute_record_length(rd, fixed, fixedNull, varlen, varlenNull);

  Record r = record_init(recordLen);
  
  fill_record(rd, r + sizeof(RecordHeader), fixed, varlen, fixedNull, varlenNull, NULL);

  int32_t bufId;
  BufTag* tag = bufdesc_new_buftag(FILE_DATA, 0);

  /*
    The first time we insert a record to the system table, it will be the
    `_tables` system table. So we simply allocate a brand new page and
    manually set the [prev|next]PageId header fields because they are not
    yet tracked by the system.
   */
  if (strcmp(t->name, "_tables") == 0) {
    bufId = bufmgr_allocate_new_page(buf, FILE_DATA);
    if (buf->bd->descArr[bufId]->tag->pageId != SYSTABLE_FIRST_PAGE_ID) return false;
    if (bufId < 0) return false;
    pageheader_init_datapage(buf->bp->pages[bufId]);
  } else {
    tag->pageId = systable_get_last_pageid(buf, "_tables");
    bufId = bufmgr_request_bufId(buf, tag);
  }

  while (bufId >= 0) {
    if (page_insert(buf->bp->pages[bufId], r, recordLen)) {
      bufdesc_set_dirty(buf->bd->descArr[bufId]);
      bufmgr_release_bufId(buf, bufId);
      break;
    }

    int32_t nextPageId = ((PageHeader*)buf->bp->pages[bufId])->nextPageId;
    int32_t oldBufId = bufId;

    if (nextPageId == 0) {
      bufId = bufmgr_page_split(buf, bufId);

      /* update `last_page_id` field in the _tables system table */
      systable_set_last_pageid(buf, "_tables", buf->bd->descArr[bufId]->tag->pageId);
    } else {
      tag->pageId = nextPageId;
      bufId = bufmgr_request_bufId(buf, tag);
    }

    bufmgr_release_bufId(buf, oldBufId);
  }

  bufdesc_free_buftag(tag);
  free_record_desc(rd);
  free(fixed);
  free(fixedNull);
  free(varlen);
  free(varlenNull);
  free(r);

  return true;
}

static void defill_systable(RecordDescriptor* rd, Record r, SysTable* t) {
  Datum* values = malloc(rd->ncols * sizeof(Datum));
  bool* isnull = malloc(rd->ncols * sizeof(bool));

  defill_record(rd, r, values, isnull);

  t->objectId = datumGetInt64(values[0]);
  t->name = datumGetString(values[1]);
  t->type = datumGetString(values[2]);
  t->firstPageId = datumGetInt32(values[3]);
  t->lastPageId = datumGetInt32(values[4]);
}

static void systable_scan(BufMgr* buf, RecordDescriptor* rd, LinkedList* rows) {
  BufTag* tag = bufdesc_new_buftag(FILE_DATA, SYSTABLE_FIRST_PAGE_ID);
  int32_t bufId = bufmgr_request_bufId(buf, tag);

  while (bufId >= 0) {
    Page pg = buf->bp->pages[bufId];
    PageHeader* pgHdr = (PageHeader*)pg;
    int numRecords = pgHdr->numRecords;

    for (int i = 0; i < numRecords; i++) {
      RecordSetRow* row = new_recordset_row(rows, rd->ncols);
      int slotPointerOffset = conf->pageSize - (sizeof(SlotPointer) * (i + 1));
      SlotPointer* sp = (SlotPointer*)(pg + slotPointerOffset);
      
      defill_record(rd, pg + sp->offset, row->values, row->isnull);
    }

    tag->pageId = pgHdr->nextPageId;
    bufmgr_release_bufId(buf, bufId);
    bufId = bufmgr_request_bufId(buf, tag);
  }

  bufdesc_free_buftag(tag);
}

int64_t systable_get_objectId(BufMgr* buf, char* tablename) {
  RecordDescriptor* rd = systable_get_record_desc();
  RecordSet* rs = new_recordset();

  systable_scan(buf, rd, rs->rows);

  int64_t objectId = -1;
  ListItem* li = rs->rows->head;
  while (li != NULL) {
    RecordSetRow* row = li->ptr;
    if (strcmp(tablename, datumGetString(row->values[1])) == 0) {
      objectId = datumGetInt64(row->values[0]);
      break;
    }

    li = li->next;
  }

  free_recordset(rs, rd);
  free_record_desc(rd);

  return objectId;
}

int32_t systable_get_first_pageid(BufMgr* buf, char* tablename) {
  RecordDescriptor* rd = systable_get_record_desc();
  RecordSet* rs = new_recordset();

  systable_scan(buf, rd, rs->rows);

  int32_t firstPageId = -1;
  ListItem* li = rs->rows->head;
  while (li != NULL) {
    RecordSetRow* row = li->ptr;
    if (strcmp(tablename, datumGetString(row->values[1])) == 0) {
      firstPageId = datumGetInt32(row->values[3]);
      break;
    }

    li = li->next;
  }

  free_recordset(rs, rd);
  free_record_desc(rd);

  return firstPageId;
}

int32_t systable_get_last_pageid(BufMgr* buf, char* tablename) {
  RecordDescriptor* rd = systable_get_record_desc();
  RecordSet* rs = new_recordset();

  systable_scan(buf, rd, rs->rows);

  if (rs->rows->numItems <= 0) {
    printf("No records in _tables\n");
    return -1;
  }

  int32_t lastPageId = -1;
  ListItem* li = rs->rows->head;
  while (li != NULL) {
    RecordSetRow* row = li->ptr;
    if (strcmp(tablename, datumGetString(row->values[1])) == 0) {
      lastPageId = datumGetInt32(row->values[4]);
      break;
    }

    li = li->next;
  }

  free_recordset(rs, rd);
  free_record_desc(rd);

  return lastPageId;
}

/**
 * @brief Updates the `first_page_id` column for a given table in the
 * _tables system table. Currently duplicates the logic of scanning
 * a table because I don't know how I'm going to implement an update
 * operation yet.
 * 
 * @param buf 
 * @param tablename 
 * @param firstPageId 
 * @return true 
 * @return false 
 */
bool systable_set_first_pageid(BufMgr* buf, char* tablename, int32_t firstPageId) {
  RecordDescriptor* rd = systable_get_record_desc();
  BufTag* tag = bufdesc_new_buftag(FILE_DATA, SYSTABLE_FIRST_PAGE_ID);
  int32_t bufId = bufmgr_request_bufId(buf, tag);

  bool updateSuccess = false;

  while (bufId >= 0 && !updateSuccess) {
    Page pg = buf->bp->pages[bufId];
    PageHeader* pgHdr = (PageHeader*)pg;
    int numRecords = pgHdr->numRecords;

    for (int i = 0; i < numRecords; i++) {
      int slotOffset = conf->pageSize - (sizeof(SlotPointer) * (i + 1));
      SlotPointer* sp = (SlotPointer*)(pg + slotOffset);

      Datum* values = malloc(sizeof(Datum) * rd->ncols);
      bool* isnull = malloc(sizeof(bool) * rd->ncols);

      defill_record(rd, pg + sp->offset, values, isnull);

      if (strcmp(tablename, datumGetString(values[1])) == 0) {
        int offset = compute_offset_to_column(rd, pg + sp->offset, 3);
        memcpy(pg + sp->offset + offset, &firstPageId, sizeof(int32_t));
        bufdesc_set_dirty(buf->bd->descArr[bufId]);
        bufmgr_release_bufId(buf, bufId);
        updateSuccess = true;
      }

      free_datum_array(rd, values);
      free(isnull);
    }

    if (!updateSuccess) {
      bufmgr_release_bufId(buf, bufId);
      tag->pageId = pgHdr->nextPageId;
      bufId = bufmgr_request_bufId(buf, tag);
    }
  }

  bufdesc_free_buftag(tag);
  free_record_desc(rd);

  return updateSuccess;
}

/**
 * @brief Updates the `last_page_id` column for a given table in the
 * _tables system table. Currently duplicates the logic of scanning
 * a table because I don't know how I'm going to implement an update
 * operation yet.
 * 
 * @param buf 
 * @param tablename 
 * @param lastPageId 
 * @return true 
 * @return false 
 */
bool systable_set_last_pageid(BufMgr* buf, char* tablename, int32_t lastPageId) {
  RecordDescriptor* rd = systable_get_record_desc();
  BufTag* tag = bufdesc_new_buftag(FILE_DATA, SYSTABLE_FIRST_PAGE_ID);
  int32_t bufId = bufmgr_request_bufId(buf, tag);

  bool updateSuccess = false;

  while (bufId >= 0 && !updateSuccess) {
    Page pg = buf->bp->pages[bufId];
    PageHeader* pgHdr = (PageHeader*)pg;
    int numRecords = pgHdr->numRecords;

    for (int i = 0; i < numRecords; i++) {
      int slotOffset = conf->pageSize - (sizeof(SlotPointer) * (i + 1));
      SlotPointer* sp = (SlotPointer*)(pg + slotOffset);

      Datum* values = malloc(sizeof(Datum) * rd->ncols);
      bool* isnull = malloc(sizeof(bool) * rd->ncols);

      defill_record(rd, pg + sp->offset, values, isnull);

      if (strcmp(tablename, datumGetString(values[1])) == 0) {
        int offset = compute_offset_to_column(rd, pg + sp->offset, 4);
        memcpy(pg + sp->offset + offset, &lastPageId, sizeof(int32_t));
        bufdesc_set_dirty(buf->bd->descArr[bufId]);
        bufmgr_release_bufId(buf, bufId);
        updateSuccess = true;
      }

      free_datum_array(rd, values);
      free(isnull);
    }

    if (!updateSuccess) {
      bufmgr_release_bufId(buf, bufId);
      tag->pageId = pgHdr->nextPageId;
      bufId = bufmgr_request_bufId(buf, tag);
    }
  }

  bufdesc_free_buftag(tag);
  free_record_desc(rd);

  return updateSuccess;
}