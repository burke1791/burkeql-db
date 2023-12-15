#include <string.h>
#include <stdlib.h>

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

static void fill_val(Column* col, char** dataP, Datum datum) {
  int16_t dataLen;
  char* data = *dataP;

  switch (col->dataType) {
    case DT_INT:
      dataLen = 4;
      int32_t valInt = datumGetInt32(datum);
      memcpy(data, &valInt, dataLen);
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

void fill_record(RecordDescriptor* rd, Record r, Datum* data) {
  for (int i = 0; i < rd->ncols; i++) {
    Column* col = &rd->cols[i];

    fill_val(col, &r, data[i]);
  }
}