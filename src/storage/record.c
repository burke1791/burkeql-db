#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "storage/record.h"

Record record_init(uint16_t recordLen) {
  Record r = malloc(recordLen);
  memset(r, 0, recordLen);

  return r;
}

void free_record(Record r) {
  if (r != NULL) free(r);
}

void construct_column_desc(Column* col, char* colname, DataType type, int colnum, int len) {
  col->colname = strdup(colname);
  col->dataType = type;
  col->colnum = colnum;
  col->len = len;
}

static Column* get_nth_col(RecordDescriptor* rd, bool isFixed, int n) {
  int nCol = 0;

  for (int i = 0; i < rd->ncols; i++) {
    if (rd->cols[i].dataType == DT_VARCHAR) {
      // column is variable-length
      if (!isFixed && nCol == n) return &rd->cols[i];
      if (!isFixed) nCol++;
    } else {
      // column is fixed length
      if (isFixed && nCol == n) return &rd->cols[i];
      if (isFixed) nCol++;
    }
  }

  return NULL;
}

static void fill_val(Column* col, char** dataP, Datum datum) {
  int16_t dataLen;
  char* data = *dataP;

  switch (col->dataType) {
    case DT_BOOL:     // Bools and TinyInts are the same C-type
    case DT_TINYINT:
      dataLen = 1;
      uint8_t valTinyInt = datumGetUInt8(datum);
      memcpy(data, &valTinyInt, dataLen);
      break;
    case DT_SMALLINT:
      dataLen = 2;
      int16_t valSmallInt = datumGetInt16(datum);
      memcpy(data, &valSmallInt, dataLen);
      break;
    case DT_INT:
      dataLen = 4;
      int32_t valInt = datumGetInt32(datum);
      memcpy(data, &valInt, dataLen);
      break;
    case DT_BIGINT:
      dataLen = 8;
      int64_t valBigInt = datumGetInt64(datum);
      memcpy(data, &valBigInt, dataLen);
      break;
    case DT_CHAR:
      dataLen = col->len;
      char* str = strdup(datumGetString(datum));
      int charLen = strlen(str);
      if (charLen > dataLen) charLen = dataLen;
      memcpy(data, str, charLen);
      free(str);
      break;
  }

  data += dataLen;
  *dataP = data;
}

/**
 * Takes a Datum array and serializes the data into a Record
 */
void fill_record(RecordDescriptor* rd, Record r, Datum* fixed, Datum* varlen) {
  for (int i = 0; i < rd->ncols; i++) {
    Column* col = &rd->cols[i];

    fill_val(col, &r, data[i]);
  }
}

static Datum record_get_tinyint(Record r, int* offset) {
  uint8_t tinyintVal;
  memcpy(&tinyintVal, r + *offset, 1);
  *offset += 1;
  return uint8GetDatum(tinyintVal);
}

static Datum record_get_smallint(Record r, int* offset) {
  int16_t smallintVal;
  memcpy(&smallintVal, r + *offset, 2);
  *offset += 2;
  return int16GetDatum(smallintVal);
}

static Datum record_get_int(Record r, int* offset) {
  int32_t intVal;
  memcpy(&intVal, r + *offset, 4);
  *offset += 4;
  return int32GetDatum(intVal);
}

static Datum record_get_bigint(Record r, int* offset) {
  int64_t bigintVal;
  memcpy(&bigintVal, r + *offset, 8);
  *offset += 8;
  return int64GetDatum(bigintVal);
}

static Datum record_get_char(Record r, int* offset, int charLen) {
  char* pChar = malloc(charLen + 1);
  memcpy(pChar, r + *offset, charLen);
  pChar[charLen] = '\0';
  *offset += charLen;
  return charGetDatum(pChar);
}

static Datum record_get_col_value(Column* col, Record r, int* offset) {
  switch (col->dataType) {
    case DT_BOOL:     // Bools and TinyInts are the same C-type
    case DT_TINYINT:
      return record_get_tinyint(r, offset);
    case DT_SMALLINT:
      return record_get_smallint(r, offset);
    case DT_INT:
      return record_get_int(r, offset);
    case DT_BIGINT:
      return record_get_bigint(r, offset);
    case DT_CHAR:
      return record_get_char(r, offset, col->len);
    default:
      printf("record_get_col_value() | Unknown data type!\n");
      return (Datum)NULL;
  }
}

/**
 * Opposite of fill_record. Deserializes data from a Record into a Datum array
 */
void defill_record(RecordDescriptor* rd, Record r, Datum* values) {
  int offset = sizeof(RecordHeader);
  for (int i = 0; i < rd->ncols; i++) {
    Column* col = &rd->cols[i];
    values[i] = record_get_col_value(col, r, &offset);
  }
}