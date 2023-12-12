#ifndef RECORD_H
#define RECORD_H

#include <stdint.h>

typedef char* Record;

typedef enum DataType {
  DT_INT = 2,     /* 4-bytes, signed */
  DT_CHAR = 7     /* Byte-size defined at table creation */
} DataType;

typedef struct Column {
  char* colname;
  DataType dataType;
  int colnum;     /* 0-based col index */
  int len;
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

#endif /* RECORD_H */