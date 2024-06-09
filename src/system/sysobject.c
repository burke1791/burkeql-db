
#include "global/config.h"
#include "system/sysobject.h"

extern Config* conf;

RecordDescriptor* sysobject_get_record_desc() {
  RecordDescriptor* rd = malloc(sizeof(RecordDescriptor) + (3 * sizeof(Column)));
  rd->ncols = 3;
  rd->nfixed = 2;

  construct_column_desc(&rd->cols[0], "object_id", DT_BIGINT, 0, 8, true);
  construct_column_desc(&rd->cols[1], "name", DT_VARCHAR, 1, 50, true);
  construct_column_desc(&rd->cols[2], "type", DT_CHAR, 2, 1, true);

  return rd;
}

static void sysobject_populate_values_arrays(
  Datum* fixed,
  bool* fixedNull,
  Datum* varlen,
  bool* varlenNull,
  SysObject* obj
) {
  fixed[0] = int64GetDatum(obj->objectId);
  fixedNull[0] = false;
  fixed[1] = charGetDatum(obj->type);
  fixedNull[1] = false;

  varlen[0] = charGetDatum(obj->name);
  varlenNull[0] = false;
}

Record sysobject_serialize_new_record(SysObject* obj) {
  RecordDescriptor* rd = systable_get_record_desc();

  Datum* fixed = malloc(sizeof(Datum) * rd->nfixed);
  bool* fixedNull = malloc(sizeof(bool) * rd->nfixed);
  Datum* varlen = malloc(sizeof(Datum) * (rd->ncols - rd->nfixed));
  bool* varlenNull = malloc(sizeof(bool) * (rd->ncols - rd->nfixed));

  systable_populate_values_arrays(fixed, fixedNull, varlen, varlenNull, obj);

  uint16_t recordLen = compute_record_length(rd, fixed, fixedNull, varlen, varlenNull);

  Record r = record_init(recordLen);
  int nullOffset = sizeof(RecordHeader) + compute_record_fixed_length(rd, fixedNull);
  ((RecordHeader*)r)->nullOffset = nullOffset;

  uint8_t* nullBitmap = r + nullOffset;
  fill_record(rd, r, fixed, varlen, fixedNull, varlenNull, nullBitmap);

  free_record_desc(rd);
  free(fixed);
  free(fixedNull);
  free(varlen);
  free(varlenNull);

  return r;
}

bool sysobjectinit_insert_record(BufMgr* buf, SysObject* obj) {
  RecordDescriptor* rd = systable_get_record_desc();

  Datum* fixed = malloc(sizeof(Datum) * rd->nfixed);
  bool* fixedNull = malloc(sizeof(bool) * rd->nfixed);
  Datum* varlen = malloc(sizeof(Datum) * (rd->ncols - rd->nfixed));
  bool* varlenNull = malloc(sizeof(bool) * (rd->ncols - rd->nfixed));

  systable_populate_values_arrays(fixed, fixedNull, varlen, varlenNull, obj);

  uint16_t recordLen = compute_record_length(rd, fixed, fixedNull, varlen, varlenNull);

  Record r = record_init(recordLen);
  int nullOffset = sizeof(RecordHeader) + compute_record_fixed_length(rd, fixedNull);
  ((RecordHeader*)r)->nullOffset = nullOffset;

  uint8_t* nullBitmap = r + nullOffset;
  fill_record(rd, r, fixed, varlen, fixedNull, varlenNull, nullBitmap);

  BufTag* tag = bufdesc_new_buftag(FILE_DATA, 2);
  int32_t bufId = bufmgr_request_bufId(buf, tag);

  // need to allocate a new page
  if (bufId < 0) {
    bufId = bufmgr_allocate_new_page(buf, FILE_DATA);
    if (bufId < 0) return false;
    if (buf->bd->descArr[bufId]->tag->pageId != tag->pageId) {
      printf("Incorrect pageId for sysobject table init\n");
      return false;
    }
  }

  while (bufId >= 0) {
    if (page_insert(buf->bp->pages[bufId], r, recordLen)) break;

    int32_t nextPageId = ((PageHeader*)buf->bp->pages[bufId])->nextPageId;

    // need to allocate a new page
    if (nextPageId == 0) {
      bufId = bufmgr_pagesplit_append(buf, tag);
      // need to keep track of the pageIds so we can set the correct
      // values in the `lastPageId` system table later on
    } else {
      tag->pageId = nextPageId;
      bufId = bufmgr_request_bufId(buf, tag);
    }
  }

  bufdesc_free_buftag(tag);
  free_record_desc(rd);
  free(fixed);
  free(fixedNull);
  free(varlen);
  free(varlenNull);

  return true;
}