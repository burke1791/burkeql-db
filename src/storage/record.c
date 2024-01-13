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

static void fill_varchar(Column* col, char* data, int16_t* dataLen, Datum value) {
  int16_t charLen = strlen(datumGetString(value));
  if (charLen > col->len) charLen = col->len;
  charLen += 2; // account for the 2-byte length overhead
  
  // write the 2-byte length overhead
  memcpy(data, &charLen, 2);

  // write the actual data
  memcpy(data + 2, datumGetString(value), charLen - 2);
  *dataLen = charLen;
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
    case DT_VARCHAR:
      fill_varchar(col, data, &dataLen, datum);
      break;
  }

  data += dataLen;
  *dataP = data;
}

/**
 * Takes a Datum array and serializes the data into a Record
 */
void fill_record(RecordDescriptor* rd, Record r, Datum* fixed, Datum* varlen) {
  // fill fixed-length columns
  for (int i = 0; i < rd->nfixed; i++) {
    Column* col = get_nth_col(rd, true, i);

    fill_val(col, &r, fixed[i]);
  }

  // fill varlen columns
  for (int i = 0; i < (rd->ncols - rd->nfixed); i++) {
    Column* col = get_nth_col(rd, false, i);

    fill_val(col, &r, varlen[i]);
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

static Datum record_get_varchar(Record r, int* offset) {
  int16_t len;
  memcpy(&len, r + *offset, 2);

  /*
    The `len - 1` expression is a combination of two steps:

    We subtract 2 bytes because we don't need memory for
    the 2-byte length overhead.

    Then we add 1 byte to account for the null terminator we
    need to append on the end of the string.
  */
  char* pChar = malloc(len - 1);
  memcpy(pChar, r + *offset + 2, len - 2);
  pChar[len - 2] = '\0';
  *offset += len;
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
    case DT_VARCHAR:
      return record_get_varchar(r, offset);
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

  Column* col;
  for (int i = 0; i < rd->ncols; i++) {
    if (i < rd->nfixed) {
      col = get_nth_col(rd, true, i);
    } else {
      col = get_nth_col(rd, false, i - rd->nfixed);
    }

    values[col->colnum] = record_get_col_value(col, r, &offset);
  }
}