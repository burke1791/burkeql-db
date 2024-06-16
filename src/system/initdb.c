#include <stdlib.h>

#include "system/initdb.h"
#include "global/config.h"
#include "system/boot.h"
#include "system/systable.h"
#include "system/syscolumn.h"
#include "system/syssequence.h"

extern Config* conf;

static bool init_table(BufMgr* buf, int64_t objectId, char* name, char* type, int32_t firstPageId, int32_t lastPageId) {
  SysTable* t = malloc(sizeof(SysTable));
  t->objectId = objectId;
  t->name = name;
  t->type = type;
  t->firstPageId = firstPageId;
  t->lastPageId = lastPageId;

  bool success = systableinit_insert_record(buf, t);
  free(t);

  return success;
}

static bool init_tables(BufMgr* buf) {
  if (!init_table(buf, 1, "_tables", "s", SYSTABLE_FIRST_PAGE_ID, SYSTABLE_FIRST_PAGE_ID)) return false;
  if (!init_table(buf, 2, "_columns", "s", 0, 0)) return false;
  if (!init_table(buf, 3, "_sequences", "s", 0, 0)) return false;

  return true;
}

static bool init_column(
  BufMgr* buf,
  int64_t objectId,
  int64_t tableId,
  char* name,
  uint8_t dataType,
  int16_t maxLen,
  uint8_t precision,
  uint8_t scale,
  uint8_t colnum,
  uint8_t isNotNull
) {
  SysColumn* c = malloc(sizeof(SysColumn));
  c->objectId = objectId;
  c->tableId = tableId;
  c->name = name;
  c->dataType = dataType;
  c->maxLen = maxLen;
  c->precision = precision;
  c->scale = scale;
  c->colnum = colnum;
  c->isNotNull = isNotNull;

  bool success = syscolumninit_insert_record(buf, c);
  free(c);

  return success;
}

static bool init_columns(BufMgr* buf) {
  if (!init_column(buf, 4, 1, "object_id", DT_BIGINT, 8, 0, 0, 0, 1)) return false;
  if (!init_column(buf, 5, 1, "name", DT_VARCHAR, 50, 0, 0, 1, 1)) return false;
  if (!init_column(buf, 6, 1, "type", DT_CHAR, 1, 0, 0, 2, 1)) return false;
  if (!init_column(buf, 7, 1, "first_page_id", DT_INT, 4, 0, 0, 3, 1)) return false;
  if (!init_column(buf, 8, 1, "last_page_id", DT_INT, 4, 0, 0, 4, 1)) return false;

  if (!init_column(buf, 9, 2, "object_id", DT_BIGINT, 8, 0, 0, 0, 1)) return false;
  if (!init_column(buf, 10, 2, "table_id", DT_BIGINT, 8, 0, 0, 1, 1)) return false;
  if (!init_column(buf, 11, 2, "name", DT_VARCHAR, 50, 0, 0, 2, 1)) return false;
  if (!init_column(buf, 12, 2, "data_type", DT_TINYINT, 1, 0, 0, 3, 1)) return false;
  if (!init_column(buf, 13, 2, "max_length", DT_SMALLINT, 2, 0, 0, 4, 1)) return false;
  if (!init_column(buf, 14, 2, "precision", DT_TINYINT, 1, 0, 0, 5, 1)) return false;
  if (!init_column(buf, 15, 2, "scale", DT_TINYINT, 1, 0, 0, 6, 1)) return false;
  if (!init_column(buf, 16, 2, "colnum", DT_TINYINT, 1, 0, 0, 7, 1)) return false;
  if (!init_column(buf, 17, 2, "is_not_null", DT_TINYINT, 1, 0, 0, 8, 1)) return false;

  if (!init_column(buf, 18, 3, "object_id", DT_BIGINT, 8, 0, 0, 0, 1)) return false;
  if (!init_column(buf, 19, 3, "name", DT_VARCHAR, 50, 0, 0, 1, 1)) return false;
  if (!init_column(buf, 20, 3, "column_id", DT_BIGINT, 8, 0, 0, 2, 1)) return false;
  if (!init_column(buf, 21, 3, "next_value", DT_BIGINT, 8, 0, 0, 3, 1)) return false;
  if (!init_column(buf, 22, 3, "increment", DT_BIGINT, 8, 0, 0, 4, 1)) return false;

  return true;
}

static bool init_sequence(
  BufMgr* buf,
  int64_t objectId,
  char* name,
  char* type,
  int64_t columnId,
  int64_t nextValue,
  int64_t increment
) {
  SysSequence* s = malloc(sizeof(SysSequence));
  s->objectId = objectId;
  s->name = name;
  s->type = type;
  s->columnId = columnId;
  s->nextValue = nextValue;
  s->increment = increment;

  bool success = syssequenceinit_insert_record(buf, s);
  free(s);

  return success;
}

static bool init_sequences(BufMgr* buf) {
  if (!init_sequence(buf, 23, "sys_object_id", "s", 0, 24, 1)) return false;

  return true;
}

/**
 * @brief initializes the database boot and system pages if necessary
 * 
 * @details First we populate the `_tables` system table. Since we don't know
 * how many pages it will consume, we defer updating the `first_page_id` and
 * `last_page_id` columns until the end of the initialization process.
 * 
 * @todo eventually use this process to read the first 128-byte chunk of the file
 * and set conf->pageSize from the boot page value. Everything after this will read
 * data in `pageSize` chunks
 * 
 * @param buf 
 */
bool initdb(BufMgr* buf) {
  BufTag* tag = bufdesc_new_buftag(FILE_DATA, BOOT_PAGE_ID);

  int32_t bufId = bufmgr_request_bufId(buf, tag);

  if (bufId >= 0) {
    // if boot page is already populated, we can return early
    if (get_major_version(buf) > 0) {
      bufmgr_release_bufId(buf, bufId);
      bufdesc_free_buftag(tag);
      return true;
    }
  }

  if (!init_boot_page(buf, tag)) {
    printf("Unable to initialize the boot page\n");
    bufdesc_free_buftag(tag);
    return false;
  }

  bufdesc_free_buftag(tag);

  // if (!init_objects(buf)) return false;
  if (!init_tables(buf)) return false;
  if (!init_columns(buf)) return false;
  if (!init_sequences(buf)) return false;

  return true;
}