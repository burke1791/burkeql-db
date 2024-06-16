#include <stdlib.h>

#include "system/syscolumn.h"
#include "system/systable.h"

RecordDescriptor* syscolumn_get_record_desc() {
  RecordDescriptor* rd = malloc(sizeof(RecordDescriptor) + (9 * sizeof(Column)));
  rd->ncols = 9;
  rd->nfixed = 8;
  rd->hasNullableColumns = true;

  construct_column_desc(&rd->cols[0], "object_id", DT_BIGINT, 0, 8, true);
  construct_column_desc(&rd->cols[1], "table_id", DT_BIGINT, 1, 8, true);
  construct_column_desc(&rd->cols[2], "name", DT_VARCHAR, 2, 50, true);
  construct_column_desc(&rd->cols[3], "data_type", DT_TINYINT, 3, 1, true);
  construct_column_desc(&rd->cols[4], "max_length", DT_SMALLINT, 4, 2, true);
  construct_column_desc(&rd->cols[5], "precision", DT_TINYINT, 5, 1, false);
  construct_column_desc(&rd->cols[6], "scale", DT_TINYINT, 6, 1, false);
  construct_column_desc(&rd->cols[7], "colnum", DT_TINYINT, 7, 1, true);
  construct_column_desc(&rd->cols[8], "is_not_null", DT_BOOL, 8, 1, true);

  return rd;
}

static void syscolumn_populate_values_arrays(
  Datum* fixed,
  bool* fixedNull,
  Datum* varlen,
  bool* varlenNull,
  SysColumn* c
) {
  fixed[0] = int64GetDatum(c->objectId);
  fixedNull[0] = false;
  fixed[1] = int64GetDatum(c->tableId);
  fixedNull[1] = false;
  fixed[2] = uint8GetDatum(c->dataType);
  fixedNull[2] = false;
  fixed[3] = int16GetDatum(c->maxLen);
  fixedNull[3] = false;

  /* currently we don't support floating point data types */
  switch (c->dataType) {
    default:
      fixed[4] = (Datum)NULL;
      fixedNull[4] = true;
      fixed[5] = (Datum)NULL;
      fixedNull[5] = true;
  }

  fixed[6] = uint8GetDatum(c->colnum);
  fixedNull[6] = false;
  fixed[7] = uint8GetDatum(c->isNotNull);
  fixedNull[7] = false;

  varlen[0] = charGetDatum(c->name);
  varlenNull[0] = false;
}

bool syscolumninit_insert_record(BufMgr* buf, SysColumn* c) {
  RecordDescriptor* rd = syscolumn_get_record_desc();

  Datum* fixed = malloc(sizeof(Datum) * rd->nfixed);
  bool* fixedNull = malloc(sizeof(bool) * rd->nfixed);
  Datum* varlen = malloc(sizeof(Datum) * (rd->ncols - rd->nfixed));
  bool* varlenNull = malloc(sizeof(bool) * (rd->ncols - rd->nfixed));

  syscolumn_populate_values_arrays(fixed, fixedNull, varlen, varlenNull, c);

  uint16_t recordLen = compute_record_length(rd, fixed, fixedNull, varlen, varlenNull);
  Record r = record_init(recordLen);
  uint16_t nullOffset = sizeof(RecordHeader) + compute_record_fixed_length(rd, fixedNull);
  ((RecordHeader*)r)->nullOffset = nullOffset;
  uint8_t* bitmap = r + nullOffset;
  fill_record(rd, r + sizeof(RecordHeader), fixed, varlen, fixedNull, varlenNull, bitmap);

  int32_t lastPageId = systable_get_last_pageid(buf, "_columns");
  int32_t bufId;
  BufTag* tag = bufdesc_new_buftag(FILE_DATA, 0);

  if (lastPageId <= 0) {
    bufId = bufmgr_allocate_new_page(buf, FILE_DATA);
    if (bufId < 0) {
      printf("Unable to allocate new page syscolumn\n");
      return false;
    }
    pageheader_init_datapage(buf->bp->pages[bufId]);
    int32_t firstPageId = buf->bd->descArr[bufId]->tag->pageId;
    systable_set_first_pageid(buf, "_columns", firstPageId);
    systable_set_last_pageid(buf, "_columns", firstPageId);
  } else {
    tag->pageId = lastPageId;
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
      systable_set_last_pageid(buf, "_columns", buf->bd->descArr[bufId]->tag->pageId);
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