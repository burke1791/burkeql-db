#include <stdlib.h>
#include <string.h>

#include "system/syssequence.h"
#include "system/systable.h"

RecordDescriptor* syssequence_get_record_desc() {
  RecordDescriptor* rd = malloc(sizeof(RecordDescriptor) + (6 * sizeof(Column)));
  rd->ncols = 6;
  rd->nfixed = 5;
  rd->hasNullableColumns = true;

  construct_column_desc(&rd->cols[0], "object_id", DT_BIGINT, 0, 8, true);
  construct_column_desc(&rd->cols[1], "name", DT_VARCHAR, 1, 50, true);
  construct_column_desc(&rd->cols[2], "type", DT_CHAR, 2, 1, true);
  construct_column_desc(&rd->cols[3], "column_id", DT_BIGINT, 3, 8, true);
  construct_column_desc(&rd->cols[4], "next_value", DT_BIGINT, 4, 8, true);
  construct_column_desc(&rd->cols[5], "increment", DT_BIGINT, 5, 8, true);

  return rd;
}

static void syssequence_populate_values_arrays(
  Datum* fixed,
  bool* fixedNull,
  Datum* varlen,
  bool* varlenNull,
  SysSequence* s
) {
  fixed[0] = int64GetDatum(s->objectId);
  fixedNull[0] = false;
  fixed[1] = charGetDatum(s->type);
  fixedNull[1] = false;

  if (strcmp(s->name, "sys_object_id") == 0) {
    fixed[2] = (Datum)NULL;
    fixedNull[2] = true;
  } else {
    fixed[2] = int64GetDatum(s->columnId);
    fixedNull[2] = false;
  }
  
  fixed[3] = int64GetDatum(s->nextValue);
  fixedNull[3] = false;
  fixed[4] = int64GetDatum(s->increment);
  fixedNull[4] = false;

  varlen[0] = charGetDatum(s->name);
  varlenNull[0] = false;
}

bool syssequenceinit_insert_record(BufMgr* buf, SysSequence* s) {
  RecordDescriptor* rd = syssequence_get_record_desc();

  Datum* fixed = malloc(sizeof(Datum) * rd->nfixed);
  bool* fixedNull = malloc(sizeof(bool) * rd->nfixed);
  Datum* varlen = malloc(sizeof(Datum) * (rd->ncols - rd->nfixed));
  bool* varlenNull = malloc(sizeof(bool) * (rd->ncols - rd->nfixed));

  syssequence_populate_values_arrays(fixed, fixedNull, varlen, varlenNull, s);

  uint16_t recordLen = compute_record_length(rd, fixed, fixedNull, varlen, varlenNull);
  Record r = record_init(recordLen);
  uint16_t nullOffset = sizeof(RecordHeader) + compute_record_fixed_length(rd, fixedNull);
  ((RecordHeader*)r)->nullOffset = nullOffset;
  uint8_t* bitmap = r + nullOffset;
  fill_record(rd, r + sizeof(RecordHeader), fixed, varlen, fixedNull, varlenNull, bitmap);

  int32_t lastPageId = systable_get_last_pageid(buf, "_sequences");
  int32_t bufId;
  BufTag* tag = bufdesc_new_buftag(FILE_DATA, 0);

  if (lastPageId <= 0) {
    bufId = bufmgr_allocate_new_page(buf, FILE_DATA);
    if (bufId < 0) {
      printf("Unable to allocate new page syssequence\n");
      return false;
    }
    pageheader_init_datapage(buf->bp->pages[bufId]);
    int32_t firstPageId = buf->bd->descArr[bufId]->tag->pageId;
    systable_set_first_pageid(buf, "_sequences", firstPageId);
    systable_set_last_pageid(buf, "_sequences", firstPageId);
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
      systable_set_last_pageid(buf, "_sequences", buf->bd->descArr[bufId]->tag->pageId);
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