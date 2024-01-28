#ifndef RECORD_H
#define RECORD_H

#include <stdint.h>
#include <stdbool.h>

#include "storage/datum.h"

typedef char* Record;

typedef enum DataType {
  DT_TINYINT,     /* 1-byte, unsigned */
  DT_SMALLINT,    /* 2-bytes, signed */
  DT_INT,         /* 4-bytes, signed */
  DT_BIGINT,      /* 8-bytes, signed */
  DT_BOOL,        /* 1-byte, unsigned | similar to DT_TINYINT, but always evaluates to 1 or 0 */
  DT_CHAR,        /* Byte-size defined at table creation */
  DT_VARCHAR,     /* Variable length. A 2-byte "header" stores the length of the column
                     followed by the actual column bytes */
  DT_UNKNOWN
} DataType;

#pragma pack(push, 1) /* disabling memory alignment because I don't want to deal with it */
typedef struct Column {
  char* colname;
  DataType dataType;
  int colnum;     /* 0-based col index */
  int len;
  bool isNotNull;
} Column;

/**
 * This is the 12-byte record header that is present on every single data record.
 * 
 * xmin       | transaction Id that inserted the record
 * xmax       | transaction Id that deleted the record
 * infomask   | bit fields containing metadata about the record (more below)
 * nullOffset | byte-offset from the beginning of the record to the start of the null bitmap
 */
#pragma pack(push, 1) /* disabling memory alignment because I don't want to deal with it */
typedef struct RecordHeader {
  uint32_t xmin;
  uint32_t xmax;
  uint16_t infomask;
  uint16_t nullOffset;
} RecordHeader;

typedef struct RecordDescriptor {
  int ncols;        /* number of columns (defined by the Create Table DDL) */
  int nfixed;       /* number of fixed-length columns */
  bool hasNullableColumns;
  Column cols[];
} RecordDescriptor;

Record record_init(uint16_t recordLen);
void free_record(Record r);

void free_record_desc(RecordDescriptor* rd);

void construct_column_desc(Column* col, char* colname, DataType type, int colnum, int len, bool isNotNull);

void fill_record(RecordDescriptor* rd, Record r, Datum* fixed, Datum* varlen, bool* fixedNull, bool* varlenNull, uint8_t* bitmap);
void defill_record(RecordDescriptor* rd, Record r, Datum* values, bool* isnull);

int compute_null_bitmap_length(RecordDescriptor* rd);
int compute_record_fixed_length(RecordDescriptor* rd, bool* fixedNull);

bool col_isnull(int colnum, uint8_t* bitmap);

#endif /* RECORD_H */