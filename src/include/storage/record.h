#ifndef RECORD_H
#define RECORD_H

#include <stdint.h>
#include <stdbool.h>

#include "storage/datum.h"

typedef char* Record;

typedef enum DataType {
  DT_TINYINT = 0,     /* 1-byte, unsigned */
  DT_SMALLINT = 1,    /* 2-bytes, signed */
  DT_INT = 2,         /* 4-bytes, signed */
  DT_BIGINT = 3,      /* 8-bytes, signed */
  DT_BOOL = 4,        /* 1-byte, unsigned | similar to DT_TINYINT, but always evaluates to 1 or 0 */
  // DT_DECIMAL = 5,     /* Storage size depends on the precision:
  //                        Precision  |  Bytes
  //                        1 - 9      |  5
  //                        10 - 19    |  9
  //                        20 - 28    |  13
  //                        29 - 38    |  17
                         
  //                        The first byte stores metadata about the number,
  //                        e.g. location of the decimal point, and the remaining
  //                        bytes store the digits as if it were a whole number */
  // DT_DATETIME = 6,    /* Undecided how to store these */
  DT_CHAR = 7,        /* Byte-size defined at table creation */
  DT_VARCHAR = 8,     /* Variable length. A 2-byte "header" stores the length of the column
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

void fill_record(RecordDescriptor* rd, Record r, Datum* fixed, Datum* varlen, bool* fixedNull, bool* varlenNull, uint8_t* nullBitmap);
void defill_record(RecordDescriptor* rd, Record r, Datum* values, bool* isnull);

int compute_null_bitmap_length(RecordDescriptor* rd);
int compute_record_length(
  RecordDescriptor* rd,
  Datum* fixed,
  bool* fixedNull,
  Datum* varlen,
  bool* varlenNull
);
int compute_record_fixed_length(RecordDescriptor* rd, bool* fixedNull);
int compute_offset_to_column(RecordDescriptor* rd, Record r, int colId);

bool col_isnull(int colnum, uint8_t* nullBitmap);

void free_datum_array(RecordDescriptor* rd, Datum* values);

#endif /* RECORD_H */